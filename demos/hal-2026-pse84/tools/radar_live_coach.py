#!/usr/bin/env python3
"""Live coach for PSE84 radar DBG UART stream.

Reads firmware DBG lines and prints practical motion guidance in real time.
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from collections import deque

import serial


DBG_RE = re.compile(
    r"DBG e=(?P<e>\d+)\s+b=(?P<b>\d+)\s+d=(?P<d>-?\d+)\s+de=(?P<de>-?\d+)\s+"
    r"abs=(?P<abs>\d+)\s+idx=(?P<idx>\d+)\s+score=(?P<score>[0-9.]+)\s+"
    r"(?:ml=(?P<ml>[A-Z_]+)\s+conf=(?P<conf>[0-9.]+)\s+)?"
    r"up=(?P<up>\d+)\s+dn=(?P<dn>\d+)\s+guard=(?P<guard>\d+)\s+"
    r"cand=(?P<cand>\d+)\s+taps=(?P<taps>\d+)"
)

TAP_COUNT_RE = re.compile(r"TAP_COUNT=(?P<count>\d+)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Live motion coach for radar DBG UART output")
    parser.add_argument("--port", default="/dev/ttyACM0", help="Serial port (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument(
        "--mode",
        choices=["swipe_down", "swipe_up", "tap"],
        default="swipe_down",
        help="Target motion to coach",
    )
    parser.add_argument("--window", type=float, default=2.0, help="Analysis window in seconds")
    return parser.parse_args()


def coach_text(mode: str, frames: list[dict], tap_counts: list[int]) -> str:
    if not frames:
        return "No DBG frames yet. Keep hand still 2s, then start motion at 5-10 cm from sensor."

    score_hits_up = sum(1 for f in frames if f["idx"] == 6 and f["score"] >= 0.08)
    score_hits_dn = sum(1 for f in frames if f["idx"] == 7 and f["score"] >= 0.08)
    low_radial = sum(1 for f in frames if f["abs"] <= 18)
    high_radial = sum(1 for f in frames if f["abs"] >= 30)
    cand_hits = sum(1 for f in frames if f["cand"] == 1)

    if mode == "swipe_down":
        if score_hits_dn >= 2 and low_radial >= len(frames) // 2:
            return "Good swipe-down pattern. Keep same distance, move straight down across radar face."
        if score_hits_up > score_hits_dn:
            return "Detected swipe-up bias. Reverse direction: move top -> bottom in one smooth pass."
        if high_radial > len(frames) // 3:
            return "Too much toward/away motion. Keep depth steady and move laterally downward."
        return "Make a clearer downward swipe: 12-20 cm travel over 0.3-0.6 s at fixed distance."

    if mode == "swipe_up":
        if score_hits_up >= 2 and low_radial >= len(frames) // 2:
            return "Good swipe-up pattern. Keep same distance, move straight up across radar face."
        if score_hits_dn > score_hits_up:
            return "Detected swipe-down bias. Reverse direction: move bottom -> top in one smooth pass."
        if high_radial > len(frames) // 3:
            return "Too much toward/away motion. Keep depth steady and move laterally upward."
        return "Make a clearer upward swipe: 12-20 cm travel over 0.3-0.6 s at fixed distance."

    # tap mode
    if tap_counts:
        return f"Tap group finalized: {tap_counts[-1]}. Repeat with same speed and 2-5 cm in/out depth."
    if cand_hits >= 1 and high_radial >= 1:
        return "Tap-like impulse seen. Add a quick retreat after each poke to finalize count."
    if high_radial == 0:
        return "Tap too soft. Use a faster in/out poke (about 2-5 cm depth, <250 ms each way)."
    return "Reduce sideways drift. Taps should be mostly radial (toward sensor then back)."


def main() -> int:
    args = parse_args()

    print(f"[coach] Opening {args.port} @ {args.baud} (mode={args.mode})")
    print("[coach] Stop with Ctrl+C")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.2)
    except Exception as exc:
        print(f"[coach] Failed to open serial port: {exc}", file=sys.stderr)
        return 1

    frame_q: deque[tuple[float, dict]] = deque()
    tap_q: deque[tuple[float, int]] = deque()
    last_coach = 0.0
    last_line = 0.0
    frame_total = 0

    try:
        while True:
            raw = ser.readline()
            now = time.time()
            if raw:
                line = raw.decode(errors="ignore").strip()
                if not line:
                    continue
                last_line = now

                m = DBG_RE.search(line)
                if m:
                    frame = {
                        "e": int(m.group("e")),
                        "b": int(m.group("b")),
                        "d": int(m.group("d")),
                        "de": int(m.group("de")),
                        "abs": int(m.group("abs")),
                        "idx": int(m.group("idx")),
                        "score": float(m.group("score")),
                        "up": int(m.group("up")),
                        "dn": int(m.group("dn")),
                        "guard": int(m.group("guard")),
                        "cand": int(m.group("cand")),
                        "taps": int(m.group("taps")),
                    }
                    frame_q.append((now, frame))
                    frame_total += 1
                else:
                    tm = TAP_COUNT_RE.search(line)
                    if tm:
                        tap_q.append((now, int(tm.group("count"))))
                        print(f"[event] TAP_COUNT={tm.group('count')}")

            while frame_q and now - frame_q[0][0] > args.window:
                frame_q.popleft()
            while tap_q and now - tap_q[0][0] > args.window:
                tap_q.popleft()

            if now - last_coach >= 1.0:
                last_coach = now
                frames = [f for _, f in frame_q]
                taps = [t for _, t in tap_q]
                fps = len(frames) / args.window
                coach = coach_text(args.mode, frames, taps)
                print(f"[coach] fps={fps:.1f} frames={len(frames)} total={frame_total} | {coach}")

                if now - last_line > 2.0:
                    print("[coach] No serial lines recently. Check cable/port or reboot board.")
    except KeyboardInterrupt:
        print("\n[coach] Stopped.")
        return 0
    finally:
        ser.close()


if __name__ == "__main__":
    raise SystemExit(main())
