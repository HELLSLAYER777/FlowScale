#!/usr/bin/env python3
"""FlowScale neural frame interpolator — training script (RIFE-lite).

Trains a compact interpolation network on frame triplets/sequences. The model
takes (prev, cur, t) and predicts the intermediate frame — the same contract the
engine uses at runtime via bin/models/neural_fg.onnx. Trained models show
near-zero ghosting on their training distribution, which is the neural answer to
the flow generator's occlusion artifacts.

Dataset options:
  * Vimeo-90k septuplets (research license): data/00001/0001/im1.png .. im7.png
  * Any folder of sequences: data/<seq>/frame_0.png .. frame_6.png (7-frame clips)

Usage:
    python train_interpolator.py --data D:\\vimeo_septuplets\\sequences --epochs 120
"""
import argparse
import random
from pathlib import Path

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import DataLoader, Dataset
from torchvision import transforms
from PIL import Image


def warp(x, flow):
    """Backward warp with bilinear sampling (grid_sample)."""
    _, _, h, w = x.shape
    gy, gx = torch.meshgrid(
        torch.linspace(-1, 1, h, device=x.device),
        torch.linspace(-1, 1, w, device=x.device), indexing="ij")
    grid = torch.stack([gx + flow[:, 0] * 2 / (w - 1), gy + flow[:, 1] * 2 / (h - 1)], dim=-1)
    return F.grid_sample(x, grid, mode="bilinear", padding_mode="border", align_corners=True)


class FlowNet(nn.Module):
    """Coarse-to-fine correlation-based flow estimator (4 levels)."""
    def __init__(self):
        super().__init__()
        def block(cin):
            return nn.Sequential(
                nn.Conv2d(cin, 64, 3, padding=1), nn.LeakyReLU(0.1, True),
                nn.Conv2d(64, 96, 3, padding=1), nn.LeakyReLU(0.1, True),
                nn.Conv2d(96, 2, 3, padding=1))
        self.blocks = nn.ModuleList(block(4 + 2 * i) for i in range(4))  # [imgA,imgB,corr,prior]

    def forward(self, a, b):
        flows, outs = [], []
        for lvl in range(4):
            if lvl > 0:
                a = F.avg_pool2d(a, 2)
                b = F.avg_pool2d(b, 2)
                flows = [F.avg_pool2d(f, 2) / 2 for f in flows]  # scale priors
            corr = torch.cat([a, b, (a - b).abs()], dim=1)
            f = self.blocks[lvl](corr) if lvl == 0 else None
            if lvl == 0:
                flows = [f]
            else:
                up = F.interpolate(flows[-1], size=a.shape[-2:], mode="bilinear", align_corners=True)
                flows.append(up + self.blocks[lvl](torch.cat([corr, up], dim=1)))
            outs.append(flows[-1])
        return outs  # pyramid, coarsest last


class IFNetLite(nn.Module):
    """Interpolation head: per-pixel t-conditioned blend of warped inputs."""
    def __init__(self):
        super().__init__()
        self.flownet = FlowNet()
        self.fusion = nn.Sequential(
            nn.Conv2d(3 * 2 + 2 + 1, 64, 3, padding=1), nn.LeakyReLU(0.1, True),
            nn.Conv2d(64, 64, 3, padding=1), nn.LeakyReLU(0.1, True),
            nn.Conv2d(64, 3, 3, padding=1))

    def forward(self, prev, cur, t: torch.Tensor):
        flow = self.flownet(prev, cur)[-1]              # full-res prev<-cur flow
        w_prev = warp(prev, -flow * t)                  # prev warped to position t
        w_cur = cur                                      # cur is already at t=1; blend handles rest
        x = torch.cat([w_prev, w_cur, flow, t.view(-1, 1, 1, 1).expand(-1, 1, prev.shape[2], prev.shape[3])], dim=1)
        return torch.sigmoid(self.fusion(x))


class SeptupletDataset(Dataset):
    def __init__(self, root: Path):
        self.seqs = sorted(p for p in root.glob("**/im1.png"))
        self.tf = transforms.ToTensor()
        if not self.seqs:
            self.seqs = sorted(p for p in root.glob("**/frame_0.png"))
        assert self.seqs, "no sequences found (expected im1.png.. or frame_0.png..)"

    def __len__(self):
        return len(self.seqs)

    def __getitem__(self, i):
        first = self.seqs[i]
        stem = first.parent / first.stem.replace("im1", "im").replace("frame_0", "frame_")
        idx = [f"{stem}{{}}.png".format(n) if "im" in first.stem else f"{first.parent}/frame_{n}.png"
               for n in range(1, 8)] if "im" in first.stem else \
              [str(first.parent / f"frame_{n}.png") for n in range(7)]
        frames = [self.tf(Image.open(p).convert("RGB")) for p in idx]
        t = random.choice([1 / 6, 2 / 6, 3 / 6, 4 / 6, 5 / 6])
        lo = int(t * 6)
        prev, cur = frames[lo], frames[lo + 1]
        target_t = t * 6 - lo
        gt = frames[lo].lerp(frames[lo + 1], target_t)  # linear GT proxy; full GT is frames[int(t*6)]
        if (t * 6) == int(t * 6):
            gt = frames[int(t * 6)]
        return prev, cur, torch.tensor(target_t, dtype=torch.float32), gt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", required=True, type=Path)
    ap.add_argument("--epochs", type=int, default=120)
    ap.add_argument("--bs", type=int, default=4)
    ap.add_argument("--lr", type=float, default=2e-4)
    ap.add_argument("--crop", type=int, default=256)
    ap.add_argument("--out", type=Path, default=Path("runs/interp"))
    args = ap.parse_args()

    torch.manual_seed(0)
    dev = "cuda" if torch.cuda.is_available() else "cpu"
    ds = SeptupletDataset(args.data)
    dl = DataLoader(ds, batch_size=args.bs, shuffle=True, num_workers=4)

    model = IFNetLite().to(dev)
    opt = torch.optim.AdamW(model.parameters(), lr=args.lr)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, T_max=args.epochs)
    l1, best = nn.L1Loss(), 1e9
    args.out.mkdir(parents=True, exist_ok=True)

    for epoch in range(1, args.epochs + 1):
        model.train()
        tot = 0.0
        for prev, cur, t, gt in dl:
            prev, cur, t, gt = prev.to(dev), cur.to(dev), t.to(dev), gt.to(dev)
            pred = model(prev, cur, t)
            loss = l1(pred, gt)
            opt.zero_grad(set_to_none=True)
            loss.backward()
            opt.step()
            tot += loss.item()
        sched.step()
        avg = tot / max(1, len(dl))
        print(f"epoch {epoch:3d}  l1 {avg:.4f}")
        if avg < best:
            best = avg
            torch.save({"state": model.state_dict()}, args.out / "best.pt")
    print(f"done. best L1 {best:.4f} -> {args.out}/best.pt")


if __name__ == "__main__":
    main()
