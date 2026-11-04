#!/usr/bin/env python3
"""Export a trained FlowScale checkpoint to ONNX for bin/models/.

    python export_onnx.py --ckpt runs/upscaler/best.pt  --kind upscale --out neural_upscale.onnx
    python export_onnx.py --ckpt runs/interp/best.pt    --kind interp   --out neural_fg.onnx
"""
import argparse
from pathlib import Path

import torch


def export_upscaler(ckpt_path: Path, out: Path):
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from train_upscaler import UpscaleNet

    ckpt = torch.load(ckpt_path, map_location="cpu")
    model = UpscaleNet(scale=ckpt.get("scale", 2)).eval()
    model.load_state_dict(ckpt["state"])

    class Wrapper(torch.nn.Module):
        def __init__(self, m):
            super().__init__()
            self.m = m

        def forward(self, src):          # 1x3xHxW -> 1x3xsHxsW
            return self.m(src)

    dummy = torch.randn(1, 3, 256, 256)
    torch.onnx.export(Wrapper(model), (dummy,), str(out), opset_version=17,
                      input_names=["src"], output_names=["dst"],
                      dynamic_axes={"src": {2: "H", 3: "W"}, "dst": {2: "sH", 3: "sW"}})
    print(f"wrote {out} (dynamic H/W)")


def export_interpolator(ckpt_path: Path, out: Path):
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from train_interpolator import IFNetLite

    ckpt = torch.load(ckpt_path, map_location="cpu")
    model = IFNetLite().eval()
    model.load_state_dict(ckpt["state"])

    dummy = (torch.randn(1, 3, 256, 256), torch.randn(1, 3, 256, 256),
             torch.tensor([0.5]))
    torch.onnx.export(model, dummy, str(out), opset_version=17,
                      input_names=["prev", "cur", "t"], output_names=["out"],
                      dynamic_axes={"prev": {2: "H", 3: "W"},
                                    "cur": {2: "H", 3: "W"},
                                    "out": {2: "H", 3: "W"}})
    print(f"wrote {out} (dynamic H/W)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ckpt", required=True, type=Path)
    ap.add_argument("--kind", choices=["upscale", "interp"], required=True)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    if args.kind == "upscale":
        export_upscaler(args.ckpt, args.out)
    else:
        export_interpolator(args.ckpt, args.out)


if __name__ == "__main__":
    main()
