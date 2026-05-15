[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/VwNt5n7h)
# 2026 OOP Assignment #5

- Due: 2026/5/12 23:59:59
- Author: 111703003 廖經翔
- Contact: <111703003@g.nccu.edu.tw>

---

## 2D Game (1/2)

Obejctive: Building a basic structure of a game and practice Object-Oriented Design

### Description

In this assignment, you are required to complete the basic game sturcture. Raylib has no OOP architecture, so you need to create a simple one by yourself.

### Requirements

1. Design an architecture that decouples UI logics and Data logics
2. Design a class that controls player's moving behavior. (e.g. WASD to move.)
3. Design a tool Class that can generate GameObjects. (Reference: [Factory Pattern](https://medium.com/@antwang/factory-method-in-c-the-right-way-e8c5f015fe39))
4. Draw a UML Class Diagram that shows the architecture of your app briefly.

### Expected Output

After running the program, the following should be displayed on the window:

1. At least one simple GameObject 
2. A movable player GameObject with key control

> You can customize every GameObject's appearance

### Clone the Dev Template

```console
git clone https://github.com/oopnccucs/raylib-dev-template.git
```

> If you name your project `OOPFinal`, enter

```console
mv raylib-dev-temlate OOPFinal && cd OOPFinal && rm -rf .git/
```

### Compile and Run

Before compiling the program, in **Project Folder**:

```console
cmake -B build
```

```console
cmake --build build
```

To run the program:

```console
./build/[your_project_name]
```
### Submit the assignment
Please Create a Github Repository of your own, and submit it's URL on Moodle.

### Scoring Criteria

| Description                               | Score |
| ----------------------------------------- | ----- |
| Submit the assignment                     | 20    |
| Able to compile                           | 20    |
| Draw GameObject correctly                 | 20    |
| UI / Data duty separation                 | 10    |
| Implement Player GameObject correctly     | 10    |
| Implement GameObjectFactory               | 10    |
| UML Class Diagram                         | 10    |


### Reminder

1. The default `include` and `resources` path is configured. You don't need to add prefix to locate files. (Just use `#include "some_header.h"` under include/)
2. You may create new files for more features.
3. This assignment is the foundation of the following one.
