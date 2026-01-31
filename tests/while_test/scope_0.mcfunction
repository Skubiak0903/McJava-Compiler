scoreboard objectives add mcjava_sb_scope_0 dummy
scoreboard players set %10 mcjava_sb_scope_0 10
#Debug: OPERATION 1 
scoreboard players operation x mcjava_sb_scope_0 = %10 mcjava_sb_scope_0
# Check condition to enter the loop
#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
scoreboard players set %0 mcjava_sb_scope_0 0
execute store success score %2 mcjava_sb_scope_0 run execute if score %x mcjava_sb_scope_0 > %0 mcjava_sb_scope_0
execute if score %2 mcjava_sb_scope_0 matches 1 run function mcjava_test:if_test/scope_1
