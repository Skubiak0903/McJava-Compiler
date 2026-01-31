scoreboard objectives add mcjava_sb_scope_0 dummy
scoreboard players set %5 mcjava_sb_scope_0 5
#Debug: OPERATION 1 
scoreboard players operation x mcjava_sb_scope_0 = %5 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
scoreboard players set %5 mcjava_sb_scope_0 5
#Debug: OPERATION 3 
scoreboard players operation %0 mcjava_sb_scope_0 = %x mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %0 mcjava_sb_scope_0 += %5 mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation x mcjava_sb_scope_0 = %0 mcjava_sb_scope_0
# Check condition  'if'
execute if function mcjava_test:test/scope_1 run function mcjava_test:test/scope_2
