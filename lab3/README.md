# Files components

`chunk_lock.c` the lock-based version for lab3 question 4
`chunk_lock_free.c` the lock-free version

# To compile and execute
```bash
gcc chunk_lock.c -o chunk_lock
./chunk_lock 12345678 > lock.data
```
```bash
gcc chunk_lock_free.c -o chunk_lock_free
./chunk_lock_free 12345678 > lock_free.data
```