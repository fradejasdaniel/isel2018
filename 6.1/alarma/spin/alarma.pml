ltl spec {
[] (((state == 0) && button) -> <> (state == 1)) &&
[] (((state == 1) && presence) -> <> (state == 1)) &&
[] (((state == 1) && button) -> <> (state == 0))
}
bit button;
bit presence;
byte state;
active proctype alarma() {
state = 0;
  do
  :: (state == 0) -> atomic {
    if
      :: button -> state = 1; button = 0
    fi
  }
  :: (state == 1) -> atomic {
    if
      :: button -> state = 0; button = 0
      :: presence -> state = 1; presence = 0;
    fi
  }
  od
}

active proctype entorno () {
  do
  :: button = 1
  :: (! button) -> skip
  :: presence = 1
  :: (! presence) -> skip
  od
}
