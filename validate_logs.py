"""
Lightweight Willow House log validator.

Usage:
- Run in the same directory that the project was executed in
- Make sure that you are logging every major event required
- The script will search for all log files in the directory

Command Line Arguments:
- --limit <number> limits the number of logs that it looks at for quick tests
- --export <filename> exports a combined log, sorted by timestamp

Note: This code might be updated throughout the project to modify or add additional verifications.
"""

from __future__ import annotations

import argparse
import csv
import glob
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Set, Tuple


# Willow house layout
WILLOW_ROOMS: Dict[str, List[str]] = {
    "Van": ["Hallway"],
    "Hallway": ["Van", "Master Bedroom", "Boy's Bedroom", "Bathroom", "Kitchen", "Basement"],
    "Master Bedroom": ["Hallway"],
    "Boy's Bedroom": ["Hallway"],
    "Bathroom": ["Hallway"],
    "Basement": ["Hallway", "Basement Hallway"],
    "Basement Hallway": ["Basement", "Right Storage Room", "Left Storage Room"],
    "Right Storage Room": ["Basement Hallway"],
    "Left Storage Room": ["Basement Hallway"],
    "Kitchen": ["Hallway", "Living Room", "Garage"],
    "Living Room": ["Kitchen"],
    "Garage": ["Kitchen", "Utility Room"],
    "Utility Room": ["Garage"],
}


@dataclass
class LogEntry:
    timestamp: int
    entity_type: str
    entity_id: int
    room: str
    device: str
    boredom: int
    fear: int
    action: str
    extra: str
    source: str
    line: int
    issues: Set[str] = field(default_factory=set)

    def to_row(self, include_issues: bool = False) -> List[str]:
        return [
            str(self.timestamp),
            self.entity_type,
            str(self.entity_id),
            self.room,
            self.device,
            str(self.boredom),
            str(self.fear),
            self.action,
            self.extra,
        ] + ([";".join(sorted(self.issues))] if include_issues else [])


@dataclass
class RoomState:
    name: str
    neighbors: List[str]
    hunters: set = field(default_factory=set)
    ghost_present: bool = False
    evidence: Dict[str, int] = field(default_factory=lambda: defaultdict(int))


@dataclass
class HunterState:
    hunter_id: int
    name: str = ""
    room: Optional[str] = None
    device: str = ""
    boredom: int = 0
    fear: int = 0
    returning: bool = False
    return_stack: List[str] = field(default_factory=list)


@dataclass
class GhostState:
    ghost_id: int
    ghost_type: str = ""
    room: Optional[str] = None
    boredom: int = 0


CHANGE_ACTIONS_HUNTER = {"MOVE", "EXIT", "INIT"}
CHANGE_ACTIONS_GHOST = {"MOVE", "EXIT", "INIT"}


def compute_room_change_timestamps(entries: List[LogEntry]) -> Set[int]:
    change_times: Set[int] = set()
    for entry in entries:
        if entry.entity_type == "hunter" and entry.action in CHANGE_ACTIONS_HUNTER:
            change_times.add(entry.timestamp)
        elif entry.entity_type == "ghost" and entry.action in CHANGE_ACTIONS_GHOST:
            change_times.add(entry.timestamp)
    return change_times


def compute_pending_evidence(entries: List[LogEntry]) -> Dict[Tuple[int, str, str], int]:
    pending: Dict[Tuple[int, str, str], int] = defaultdict(int)
    for entry in entries:
        if entry.entity_type == "ghost" and entry.action == "EVIDENCE":
            key = (entry.timestamp, entry.room, entry.extra)
            pending[key] += 1
    return pending


def parse_logs(limit: Optional[int] = None) -> List[LogEntry]:
    entries: List[LogEntry] = []
    try:
        for path in sorted(glob.glob("log_*.csv")):
            with open(path, "r", encoding="utf-8", newline="") as handle:
                reader = csv.reader(handle)
                for line_number, row in enumerate(reader, start=1):
                    if not row:
                        continue

                    timestamp = int(row[0])
                    entity_type = row[1].strip()
                    entity_id = int(row[2])
                    room = row[3].strip()
                    device = row[4].strip()
                    boredom = int(row[5])
                    fear = int(row[6])
                    action = row[7].strip()
                    extra = row[8].strip()

                    entries.append(
                        LogEntry(
                            timestamp=timestamp,
                            entity_type=entity_type,
                            entity_id=entity_id,
                            room=room,
                            device=device,
                            boredom=boredom,
                            fear=fear,
                            action=action,
                            extra=extra,
                            source=path,
                            line=line_number,
                        )
                    )
    except Exception:
        print("Something was wrong while parsing.")
        raise

    entries.sort(key=lambda entry: entry.timestamp)
    if limit is not None:
        entries = entries[:limit]
    return entries


def simulate(
    entries: List[LogEntry],
    change_timestamps: Set[int],
    pending_evidence: Dict[Tuple[int, str, str], int],
) -> (Dict[str, int], Dict[str, List[str]]):
    rooms = {name: RoomState(name=name, neighbors=neighbors) for name, neighbors in WILLOW_ROOMS.items()}
    hunters: Dict[int, HunterState] = {}
    ghosts: Dict[int, GhostState] = {}

    stats = defaultdict(int)
    samples: Dict[str, List[str]] = defaultdict(list)

    def report(issue: str, entry: LogEntry, detail: str) -> None:
        stats[issue] += 1
        if len(samples[issue]) < 5:
            samples[issue].append(f"{entry.timestamp} | {detail}")
        entry.issues.add(issue)

    for index, entry in enumerate(entries):
        if entry.entity_type == "hunter":
            state = hunters.get(entry.entity_id)

            if entry.action == "INIT":
                state = HunterState(
                    hunter_id=entry.entity_id,
                    name=entry.extra,
                    room=entry.room,
                    device=entry.device,
                    boredom=entry.boredom,
                    fear=entry.fear,
                    returning=False,
                )
                hunters[entry.entity_id] = state
                if entry.room in rooms:
                    rooms[entry.room].hunters.add(entry.entity_id)
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} unknown room '{entry.room}' during INIT")
                continue

            if state is None:
                report("missing_init", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} seen before INIT")
                continue

            state.boredom = entry.boredom
            state.fear = entry.fear

            if entry.action == "MOVE":
                from_room = entry.room
                to_room = entry.extra

                if state.room != from_room:
                    report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} expected in {state.room}, log shows {from_room}")

                if from_room not in rooms or to_room not in rooms:
                    report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} unknown room in move {from_room}->{to_room}")
                else:
                    if to_room not in rooms[from_room].neighbors:
                        report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} invalid edge {from_room}->{to_room}")

                    if entry.entity_id in rooms[from_room].hunters:
                        rooms[from_room].hunters.remove(entry.entity_id)
                    else:
                        report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} not recorded in {from_room} before move")

                    rooms[to_room].hunters.add(entry.entity_id)

                if state.returning:
                    if state.return_stack:
                        expected = state.return_stack.pop()
                        if expected != to_room:
                            report("return", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} expected {expected} on return, got {to_room}")
                    else:
                        if to_room != "Van":
                            report("return", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} return stack empty but moved to {to_room}")
                else:
                    if from_room:
                        state.return_stack.append(from_room)

                state.room = to_room
                if to_room == "Van":
                    state.return_stack.clear()

            elif entry.action == "EVIDENCE":
                room = entry.room
                device = entry.device

                if device and device != state.device:
                    report("evidence", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} logged device {device} but state has {state.device}")

                if room != "Van":
                    state.returning = True

                if room in rooms:
                    if rooms[room].evidence[device] > 0:
                        rooms[room].evidence[device] -= 1
                    else:
                        key = (entry.timestamp, room, device)
                        if pending_evidence.get(key, 0) > 0:
                            pending_evidence[key] -= 1
                        else:
                            report("evidence", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} collected {device} but room missing evidence")
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} evidence in unknown room {room}")

            elif entry.action == "SWAP":
                if "->" in entry.extra:
                    _, to_device = entry.extra.split("->", 1)
                    state.device = to_device.strip()

            elif entry.action == "RETURN_START":
                if state.room != "Van":
                    state.returning = True

            elif entry.action == "RETURN_COMPLETE":
                if state.room != "Van":
                    report("return", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} completed return outside van in {state.room}")
                if state.return_stack:
                    report("return", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} return stack not empty on completion")
                state.return_stack.clear()
                state.returning = False

            elif entry.action == "EXIT":
                room = entry.room
                if room in rooms and entry.entity_id in rooms[room].hunters:
                    rooms[room].hunters.remove(entry.entity_id)
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} exit from room without occupancy ({room})")
                state.room = None
                state.return_stack.clear()
                state.returning = False

            # Boredom reset check
            ghost_state = next(iter(ghosts.values()), None)
            if (
                ghost_state
                and ghost_state.room
                and state.room == ghost_state.room
                and state.boredom != 0
                and entry.timestamp not in change_timestamps
            ):
                report("boredom", entry, f"{entry.source}:{entry.line} hunter {entry.entity_id} boredom {state.boredom} with ghost in {state.room}")

        elif entry.entity_type == "ghost":
            state = ghosts.get(entry.entity_id)

            if entry.action == "INIT":
                state = GhostState(
                    ghost_id=entry.entity_id,
                    ghost_type=entry.extra,
                    room=entry.room,
                    boredom=entry.boredom,
                )
                ghosts[entry.entity_id] = state
                if entry.room in rooms:
                    rooms[entry.room].ghost_present = True
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} init unknown room {entry.room}")
                continue

            if state is None:
                report("missing_init", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} seen before INIT")
                continue

            state.boredom = entry.boredom

            if entry.action == "MOVE":
                from_room = entry.room
                to_room = entry.extra

                if state.room != from_room:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} expected in {state.room}, log shows {from_room}")

                if from_room in rooms:
                    rooms[from_room].ghost_present = False
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} left unknown room {from_room}")

                if to_room in rooms:
                    rooms[to_room].ghost_present = True
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} entered unknown room {to_room}")

                if from_room not in rooms or to_room not in rooms or to_room not in rooms[from_room].neighbors:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} invalid edge {from_room}->{to_room}")

                state.room = to_room

            elif entry.action == "EVIDENCE":
                room = entry.room
                device = entry.extra
                if room in rooms:
                    rooms[room].evidence[device] += 1
                    key = (entry.timestamp, room, device)
                    if pending_evidence.get(key, 0) > 0:
                        pending_evidence[key] -= 1
                else:
                    report("movement", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} dropped evidence in unknown room {room}")

            elif entry.action == "EXIT":
                room = entry.room
                if room in rooms:
                    rooms[room].ghost_present = False
                state.room = None

            if state.room and state.room in rooms:
                hunters_here = rooms[state.room].hunters
                if hunters_here and state.boredom != 0 and entry.timestamp not in change_timestamps:
                    report("boredom", entry, f"{entry.source}:{entry.line} ghost {entry.entity_id} boredom {state.boredom} with hunters in {state.room}")

        else:
            report("unknown_entity", entry, f"{entry.source}:{entry.line} unknown entity type '{entry.entity_type}'")

    stats["entries"] = len(entries)
    return stats, samples


def export_entries(entries: List[LogEntry], path: str) -> None:
    with open(path, "w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "timestamp",
            "entity_type",
            "entity_id",
            "room",
            "device",
            "boredom",
            "fear",
            "action",
            "extra",
            "issues",
        ])
        for entry in entries:
            writer.writerow(entry.to_row(include_issues=True))


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate Willow House log files.")
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Process at most this many log entries (after sorting).",
    )
    parser.add_argument(
        "--export",
        type=str,
        default=None,
        help="Optional output CSV path containing the combined, ordered logs.",
    )

    args = parser.parse_args()

    entries = parse_logs(limit=args.limit)
    change_timestamps = compute_room_change_timestamps(entries)
    pending_evidence = compute_pending_evidence(entries)
    stats, samples = simulate(entries, change_timestamps, pending_evidence)

    print(f"Processed entries: {stats['entries']}")
    print(f"Movement issues: {stats['movement']}")
    if samples["movement"]:
        for sample in samples["movement"]:
            print(f"  - {sample}")
    print(f"Return path issues: {stats['return']}")
    if samples["return"]:
        for sample in samples["return"]:
            print(f"  - {sample}")
    print(f"Evidence issues: {stats['evidence']}")
    if samples["evidence"]:
        for sample in samples["evidence"]:
            print(f"  - {sample}")
    print(f"Boredom mismatches: {stats['boredom']}")
    if samples["boredom"]:
        for sample in samples["boredom"]:
            print(f"  - {sample}")
    print(f"Missing initialisation entries: {stats['missing_init']}")
    if samples["missing_init"]:
        for sample in samples["missing_init"]:
            print(f"  - {sample}")
    if stats["unknown_entity"]:
        print(f"Unknown entity lines: {stats['unknown_entity']}")
        for sample in samples["unknown_entity"]:
            print(f"  - {sample}")

    if args.export:
        export_entries(entries, args.export)
        print(f"Combined timeline exported to {args.export}")


if __name__ == "__main__":
    main()
