# CRUX2

Assembly like interpreter
  
[see examples](examples/)

## Building

*Requires **GCC**

Edit VM specs in [vm.h](include/vm.h)  
default specs:  
- REG: `4`
- MEM: `256`

```sh
git clone https://github.com/hugebrain16/crux2
./build.sh
```
  
Outputs **crux.out** if successful

## Usage

```
crux.out program.cx
```
