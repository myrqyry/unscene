#pragma once
enum class SnapshotMode {CachedOnly,IdleOnly};
struct OutputState {bool streaming=false,recording=false,replay=false,virtualCamera=false;};
inline bool canCapture(SnapshotMode mode,OutputState s){
 return mode==SnapshotMode::IdleOnly&&!s.streaming&&!s.recording&&!s.replay&&!s.virtualCamera;
}
