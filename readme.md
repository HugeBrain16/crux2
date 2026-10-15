# CRUX2

Assembly like interpreter
  
[see examples](examples/)

## Building

*Default compiler is **GCC**. If you wanna use another compiler, edit [build.sh](build.sh)

Edit VM specs in [vm.h](include/vm.h)  
default specs:  
- REG: `32`
- MEM: `65536`

```sh
git clone https://github.com/hugebrain16/crux2
./build.sh
```
  
Outputs **crux.out** if successful

## Usage

```
crux.out program.cx
```
