# Loop Body
#Debug: OPERATION 2 
scoreboard players operation %firstTerm mcjava_sb_scope_0 = firstTerm mcjava_sb_scope_0
tellraw @a [{"score":{"name":"%firstTerm","objective":"mcjava_sb_scope_0"}},]
#Debug: OPERATION 2 
scoreboard players operation %firstTerm mcjava_sb_scope_0 = firstTerm mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %secondTerm mcjava_sb_scope_0 = secondTerm mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %0 mcjava_sb_scope_0 = %firstTerm mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %0 mcjava_sb_scope_0 += %secondTerm mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation nextTerm mcjava_sb_scope_0 = %0 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %secondTerm mcjava_sb_scope_0 = secondTerm mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation firstTerm mcjava_sb_scope_0 = %secondTerm mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %nextTerm mcjava_sb_scope_0 = nextTerm mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation secondTerm mcjava_sb_scope_0 = %nextTerm mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %i mcjava_sb_scope_0 = i mcjava_sb_scope_0
scoreboard players set %1 mcjava_sb_scope_0 1
#Debug: OPERATION 3 
scoreboard players operation %1 mcjava_sb_scope_0 = %i mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %1 mcjava_sb_scope_0 += %1 mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation i mcjava_sb_scope_0 = %1 mcjava_sb_scope_0
# Recheck condition at the end of the loop
#Debug: OPERATION 2 
scoreboard players operation %i mcjava_sb_scope_0 = i mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %n mcjava_sb_scope_0 = n mcjava_sb_scope_0
execute store success score %2 mcjava_sb_scope_0 run execute if score %i mcjava_sb_scope_0 <= %n mcjava_sb_scope_0
execute if score %2 mcjava_sb_scope_0 matches 1 run function mcjava_test:fibonnaci_numbers/scope_1
