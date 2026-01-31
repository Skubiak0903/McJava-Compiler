#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
scoreboard players set %5 mcjava_sb_scope_0 5
execute store success score %2 mcjava_sb_scope_0 run execute if score %x mcjava_sb_scope_0 = %5 mcjava_sb_scope_0
# Then Body
execute unless score %2 mcjava_sb_scope_0 matches 1 run return 1
tellraw @a [{"text":"X is equal to 5"},]
