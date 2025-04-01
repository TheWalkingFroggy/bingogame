# Bingo Game Terminal
A program that simulates a random bingo game between n players in a UNIX shell, with the use of threads in C.

# Requirements
<div>
  <p dir="auto">Be sure to have:</p>
  <ul dir="auto">
    <li>
      A <b><i>UNIX</i></b> environment (still need to test in Windows);
    </li>
    <li>
      C/C++ compiler (example: <b><i>GCC</i></b>)
    </li>
  </ul>
</div>

# How does it work
<div>
  <ul dir="auto">
    <li>This is just an example project born as an exercise.</li>
    <li>The program takes <b><i>n players</i></b> and <b><i>m cards</i></b> from input shell and simulates a bingo game with random numbers extraction.</li>
    <li>More info <a href="https://github.com/TheWalkingFroggy/bingogame/blob/main/Exercise.txt"><i>here</i></a></li>
  </ul>
</div>

# Compiling and usage
<div>
  <p dir="auto">Get <b><i>bingo.c</i></b> and put it in a folder. Open a UNIX shell and navigate in that folder. Compile the file with:</p>
  <pre>gcc -g -pthread bingo.c -o bingogame</pre>
  <p dir="auto">This will generate a new <b><i>bingogame</i></b> file. Execute it with:</p>
  <pre>./bingogame n m</pre>
  <p dir="auto">where <b><i>n</i></b> is the number of players, <b><i>m</i></b> is the number of cards for each player.</p>
</div>
