#!/usr/bin/env python3
"""Create a fixed-shape identity ONNX model for backend smoke tests."""

from __future__ import annotations

import argparse
from pathlib import Path


def create_identity_model(output: Path, channels: int, frames: int, opset: int) -> None:
    try:
        import onnx
        from onnx import TensorProto, checker, helper
    except ImportError as exc:
        raise SystemExit(
            "Missing Python package 'onnx'. Install it with: "
            "py -m pip install -r tools/requirements-model.txt"
        ) from exc

    input_info = helper.make_tensor_value_info(
        "audio_in", TensorProto.FLOAT, [1, channels, frames]
    )
    output_info = helper.make_tensor_value_info(
        "audio_out", TensorProto.FLOAT, [1, channels, frames]
    )
    node = helper.make_node("Identity", ["audio_in"], ["audio_out"], name="identity_audio")
    graph = helper.make_graph(
        [node], "llvc_identity_audio", [input_info], [output_info]
    )
    model = helper.make_model(
        graph,
        producer_name="llvc",
        opset_imports=[helper.make_operatorsetid("", opset)],
    )

    checker.check_model(model)
    output.parent.mkdir(parents=True, exist_ok=True)
    onnx.save(model, output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        default="models/identity_audio_1x1x128.onnx",
        help="Output ONNX path.",
    )
    parser.add_argument("--channels", type=int, default=1)
    parser.add_argument("--frames", type=int, default=128)
    parser.add_argument("--opset", type=int, default=13)
    args = parser.parse_args()

    if args.channels <= 0 or args.frames <= 0:
        parser.error("--channels and --frames must be positive")

    output = Path(args.output)
    create_identity_model(output, args.channels, args.frames, args.opset)
    print(f"Wrote {output} with shape [1,{args.channels},{args.frames}]")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
