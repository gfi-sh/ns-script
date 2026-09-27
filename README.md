# ns
peak interpreted programming language written in C++

## usage

```bash
g++ main.cpp -o main
```

## syntax

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

println "done counting to 100"

repeat 100:
  print "A"

set u to ""
prompt "enter your name: " -> u

repeat 9999999999999999999999999999999999999999999999
  print u
```
