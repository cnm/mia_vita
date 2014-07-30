
    O  O  O  O
    |  |  |  |
    |  |  |  Yellow Led 4
    |  |  Yellow Led 3
    |  Green Led 2
    Green Led 1

States

1. Card booted, did not start any miavita stuff (network.sh)

    Led 1 On
    Led 2 Off
    Led 3 On
    Led 4 On

2. Network.sh started booting miavita stuff

    Led 1 On
    Led 2 On
    Led 3 On
    Led 4 On

3. Gps is being read
    Led 1 On
    Led 2 Off
    Led 3 On
    Led 4 Off

4. Gps is not found (this states only lasts 2 seconds) then returns to state 3

    Led 1 On
    Led 2 Off
    Led 3 Off
    Led 4 On

5. Gps is found

    Led 1 On
    Led 2 On
    Led 3 Off
    Led 4 Off
