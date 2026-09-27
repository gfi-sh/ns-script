# ns
peak interpreted programming language written in C++

## usage

```bash
g++ main.cpp -o ns
```

```bash
# run a file
./ns script.ns

# repl
./ns

# run code directly
./ns -c "println \"Hello, World!\""
```

## syntax
example script demonstrating all of the functions/keywords currently in ns

```
set value1 to 0
set value2 to 100

# comments on empty lines only
while value1 > value2
  # you MUST use double-spaced indents
  increment value1

  # ns doesn't support string concatination
  print "The current value is: "
  println value1

  if value == 21
    println "noice"

println "done counting to 100"

repeat 100:
  print "A"

set u to ""
prompt "enter your name: " -> u

repeat 9
  print u

set number1 to 6
set number2 to 2

calculate number1/number2 -> number3

if 2.01 > number3
  print "yes"
if number3 > 2.01
  print "no"
```
