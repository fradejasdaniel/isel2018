#define timeout true

ltl spec {
[] (((state == 0) && correct_code) -> <> (state == 1)) &&
[] (((state == 0) && incorrect_code) -> <> (state == 0)) &&
[] (((state == 1) && presence) -> <> (state == 1)) &&
[] (((state == 1) && timeout) -> <> (state == 1)) &&
[] (((state == 1) && correct_code) -> <> (state == 0))
[] (((state == 1) && incorrect_code) -> <> (state == 1))
}
bit correct_code;
bit incorrect_code;
bit presence;
byte state;
active proctype alarma() {
state = 0;
  do
  :: (state == 0) -> atomic {
    if
      :: correct_code -> state = 1; correct_code = 0
      :: incorrect_code -> state = 0; incorrect_code = 0
    fi
  }
  :: (state == 1) -> atomic {
    if
      :: correct_code -> state = 0; correct_code = 0
      :: presence -> state = 1; presence = 0
      :: incorrect_code -> state = 1; incorrect_code = 0
    fi
  }
  od
}

active proctype entorno () {
  do
  :: correct_code = 1
  :: (! correct_code) -> skip
  :: incorrect_code = 1
  :: (! incorrect_code) -> skip
  :: presence = 1
  :: (! presence) -> skip
  od
}
