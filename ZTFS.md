# ZTFS Documentation

# Useful things to note:
- The block size MUST be a multiple of 1024, and not lower. 4096 is typical, and ideal. Other values may have unintended consequences, as they are not error checked.

- The max file size given a block size of 4096 is 4GiB.

- The minimum allowed filesystem size is 4MiB. 

