
open-$(DIR): $(SRC-$(DIR))
close-$(DIR): $(SRC-$(DIR))

# Pop State
DIR := $(DIR_$(x))
x := $(x:%.x=%)
