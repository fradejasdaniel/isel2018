#define timeout true

ltl spec {
[] (((state == 0) && button) -> <> (state == 1)) &&
[] (((state == 1) && presence) -> <> (state == 1)) &&
[] (((state == 1) && button) -> <> (state == 2)) &&
[] (((state == 2) && button) -> <> (state == 3)) &&
[] (((state == 2) && timeout) -> <> (state == 1)) &&
[] (((state == 3) && button) -> <> (state == 4)) &&
[] (((state == 3) && timeout) -> <> (state == 1)) &&
[] (((state == 4) && button) -> <> (state == 0)) &&
[] (((state == 4) && timeout) -> <> (state == 1))
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
      :: presence -> state = 1; presence = 0
    fi
  }
  :: (state == 2) -> atomic {
    if
      :: button -> state = 3; button = 0
      :: timeout -> state = 1
    fi
  }
  :: (state == 3) -> atomic {
    if
      :: button -> state = 4; button = 0
      :: timeout -> state = 1
    fi
  }
  :: (state == 4) -> atomic {
    if
      :: button -> state = 0; button=0
      :: timeout -> state = 1
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
