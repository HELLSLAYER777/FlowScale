#!/usr/bin/env python3
"""FlowScale neural upscaler — training script.

Trains a compact residual network (EDSR-lite, <2M params) for 2x/3x/4x game-footage
upscaling, exportable to ONNX for use as bin/models/neural_upscale.onnx.

Dataset layout (pairs of stills):
    data/
      low/  img_0000.png ...      # low-res input (any size, multiples of 16)
      high/ img_0000.png ...      # matching high-res ground truth

Usage:
    python train_upscaler.py --data D:\\dataset --epochs 60 --scale 2 --bs 8
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


class ResBlock(nn.Module):
    def __init__(self, ch: int):
        super().__init__()
        self.body = nn.Sequential(
            nn.Conv2d(ch, ch, 3, padding=1), nn.ReLU(inplace=True),
            nn.Conv2d(ch, ch, 3, padding=1),
        )

    def forward(self, x):
        return x + self.body(x)


class UpscaleNet(nn.Module):
    """EDSR-lite: head -> 8 residual blocks -> pixel-shuffle upscaler."""
    def __init__(self, ch: int = 48, num_blocks: int = 8, scale: int = 2):
        super().__init__()
        self.head = nn.Conv2d(3, ch, 3, padding=1)
        self.body = nn.Sequential(*(ResBlock(ch) for _ in range(num_blocks)))
        self.tail = nn.Conv2d(ch, ch, 3, padding=1)
        self.up = nn.Sequential(
            nn.Conv2d(ch, ch * scale * scale, 3, padding=1),
            nn.PixelShuffle(scale),
            nn.Conv2d(ch, 3, 3, padding=1),
        )

    def forward(self, x):
        x = self.head(x)
        x = x + self.tail(self.body(x))
        return torch.clamp(self.up(x), 0.0, 1.0)


class PairDataset(Dataset):
    def __init__(self, root: Path, scale: int, crop: int = 128):
        self.low = sorted((root / "low").glob("*.png")) + sorted((root / "low").glob("*.jpg"))
        self.high = sorted((root / "high").glob("*.png")) + sorted((root / "high").glob("*.jpg"))
        assert len(self.low) == len(self.high) and self.low, "dataset must contain matched low/ and high/ images"
        self.scale, self.crop = scale, crop
        self.tf = transforms.ToTensor()

    def __len__(self):
        return len(self.low)

    def __getitem__(self, i):
        lr = self.tf(Image.open(self.low[i]).convert("RGB"))
        hr = self.tf(Image.open(self.high[i]).convert("RGB"))
        # Random aligned crop (multiples of the scale factor).
        c = self.crop
        x = random.randrange(0, lr.shape[2] - c + 1) if lr.shape[2] > c else 0
        y = random.randrange(0, lr.shape[1] - c + 1) if lr.shape[1] > c else 0
        lr = lr[:, y:y + c, x:x + c]
        hr = hr[:, y * self.scale:(y + c) * self.scale, x * self.scale:(x + c) * self.scale]
        return lr, hr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", required=True, type=Path)
    ap.add_argument("--epochs", type=int, default=60)
    ap.add_argument("--scale", type=int, default=2, choices=[2, 3, 4])
    ap.add_argument("--bs", type=int, default=8)
    ap.add_argument("--lr", type=float, default=2e-4)
    ap.add_argument("--out", type=Path, default=Path("runs/upscaler"))
    args = ap.parse_args()

    torch.manual_seed(0)
    dev = "cuda" if torch.cuda.is_available() else "cpu"
    ds = PairDataset(args.data, args.scale)
    dl = DataLoader(ds, batch_size=args.bs, shuffle=True, num_workers=4, pin_memory=True)

    model = UpscaleNet(scale=args.scale).to(dev)
    opt = torch.optim.AdamW(model.parameters(), lr=args.lr)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, T_max=args.epochs)
    scaler = torch.amp.GradScaler(enabled=dev == "cuda")
    l1 = nn.L1Loss()

    best = 1e9
    args.out.mkdir(parents=True, exist_ok=True)
    for epoch in range(1, args.epochs + 1):
        model.train()
        tot = 0.0
        for lr_img, hr_img in dl:
            lr_img, hr_img = lr_img.to(dev), hr_img.to(dev)
            with torch.autocast(dev, enabled=dev == "cuda"):
                loss = l1(model(lr_img), hr_img)
            opt.zero_grad(set_to_none=True)
            scaler.scale(loss).backward()
            scaler.step(opt)
            scaler.update()
            tot += loss.item()
        sched.step()
        avg = tot / max(1, len(dl))
        print(f"epoch {epoch:3d}  l1 {avg:.4f}")
        if avg < best:
            best = avg
            torch.save({"state": model.state_dict(), "scale": args.scale},
                       args.out / "best.pt")
    print(f"done. best L1 {best:.4f} -> {args.out}/best.pt")


if __name__ == "__main__":
    main()
