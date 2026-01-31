# Loop Body
#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
scoreboard players set %1 mcjava_sb_scope_0 1
#Debug: OPERATION 3 
scoreboard players operation %0 mcjava_sb_scope_0 = %x mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %0 mcjava_sb_scope_0 -= %1 mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation x mcjava_sb_scope_0 = %0 mcjava_sb_scope_0
# Recheck condition at the end of the loop
#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
scoreboard players set %0 mcjava_sb_scope_0 0
execute store success score %1 mcjava_sb_scope_0 run execute if score %x mcjava_sb_scope_0 > %0 mcjava_sb_scope_0
execute if score %1 mcjava_sb_scope_0 matches 1 run function mcjava_test:if_test/scope_1
