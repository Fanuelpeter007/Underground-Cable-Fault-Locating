"""Detect cable fault thumps in live audio on a Raspberry Pi.

The STM32 line output (the cleaned signal) goes into a USB sound card on the
Pi. This script feeds that audio to an Edge Impulse audio model (MFCC features
and a CNN) and shows on screen when a thump is detected.

Usage:
    python detect_thump.py --model model.eim
    python detect_thump.py --model model.eim --device 1 --label Thumping --threshold 0.8

The model has two classes, "Noise" and "Thumping". It uses 13 MFCC
coefficients per frame (2,600 input features) and two 1D convolution layers.

Export the model from Edge Impulse as "Linux (AARCH64)" to get the .eim file.
Run with --list-devices to see the audio input IDs.
"""

import argparse
import os
import signal
import sys
import time

from edge_impulse_linux.audio import AudioImpulseRunner


def parse_args():
    """Read command line options."""
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--model", help="Path to the Edge Impulse .eim model file")
    p.add_argument("--device", type=int, default=None,
                   help="Audio input device ID (default: ask interactively)")
    p.add_argument("--label", default="Thumping",
                   help="Model label that means a thump (default: Thumping)")
    p.add_argument("--threshold", type=float, default=0.8,
                   help="Minimum score to report a detection, 0 to 1 (default: 0.8)")
    p.add_argument("--list-devices", action="store_true",
                   help="List audio input devices and exit")
    return p.parse_args()


def list_devices():
    """Print the audio input devices PyAudio can see."""
    import pyaudio  # imported here so --help works without audio hardware

    pa = pyaudio.PyAudio()
    try:
        for i in range(pa.get_device_count()):
            info = pa.get_device_info_by_index(i)
            if info.get("maxInputChannels", 0) > 0:
                print(f"{i}: {info['name']} ({int(info['defaultSampleRate'])} Hz)")
    finally:
        pa.terminate()


def show(label, score, detected):
    """Print one status line. A detection is shown in capitals so it is easy to read on the Pi screen."""
    stamp = time.strftime("%H:%M:%S")
    if detected:
        print(f"[{stamp}] THUMP DETECTED  {label} = {score:.0%}", flush=True)
    else:
        print(f"[{stamp}] listening...    {label} = {score:.0%}", flush=True)


def main():
    args = parse_args()

    if args.list_devices:
        list_devices()
        return 0

    if not args.model:
        print("error: --model is required", file=sys.stderr)
        return 2

    model_path = os.path.abspath(args.model)
    if not os.path.isfile(model_path):
        print(f"error: model file not found: {model_path}", file=sys.stderr)
        return 2

    # The runner only streams results while it is used as a context manager.
    with AudioImpulseRunner(model_path) as runner:
        signal.signal(signal.SIGINT, lambda *_: (runner.stop(), sys.exit(0)))
        try:
            model_info = runner.init()
            labels = model_info["model_parameters"]["labels"]
            print(f"Loaded {model_info['project']['name']}, labels: {labels}")
            if args.label not in labels:
                print(f"error: label '{args.label}' is not in the model. Use --label.",
                      file=sys.stderr)
                return 2

            for result, _audio in runner.classifier(device_id=args.device):
                score = result["result"]["classification"][args.label]
                show(args.label, score, score >= args.threshold)
        finally:
            runner.stop()

    return 0


if __name__ == "__main__":
    sys.exit(main())
