scoreboard objectives add mcjava_sb_scope_0 dummy
scoreboard players set %10000 mcjava_sb_scope_0 10000
#Debug: OPERATION 1 
scoreboard players operation n mcjava_sb_scope_0 = %10000 mcjava_sb_scope_0
scoreboard players set %1 mcjava_sb_scope_0 1
#Debug: OPERATION 1 
scoreboard players operation i mcjava_sb_scope_0 = %1 mcjava_sb_scope_0
scoreboard players set %0 mcjava_sb_scope_0 0
#Debug: OPERATION 1 
scoreboard players operation firstTerm mcjava_sb_scope_0 = %0 mcjava_sb_scope_0
scoreboard players set %1 mcjava_sb_scope_0 1
#Debug: OPERATION 1 
scoreboard players operation secondTerm mcjava_sb_scope_0 = %1 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %n mcjava_sb_scope_0 = n mcjava_sb_scope_0
tellraw @a [{"text":"Fibonacci Numbers ("},{"score":{"name":"%n","objective":"mcjava_sb_scope_0"}},{"text":" iterations):"},]
# Check condition to enter the loop
#Debug: OPERATION 2 
scoreboard players operation %i mcjava_sb_scope_0 = i mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %n mcjava_sb_scope_0 = n mcjava_sb_scope_0
execute store success score %3 mcjava_sb_scope_0 run execute if score %i mcjava_sb_scope_0 <= %n mcjava_sb_scope_0
execute if score %3 mcjava_sb_scope_0 matches 1 run function mcjava_test:fibonnaci_numbers/scope_1
