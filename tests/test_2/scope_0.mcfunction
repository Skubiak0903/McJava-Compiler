scoreboard objectives add mcjava_sb_scope_0 dummy
# Scope {
scoreboard players set %5 mcjava_sb_scope_0 5
#Debug: OPERATION 1 
scoreboard players operation a mcjava_sb_scope_0 = %5 mcjava_sb_scope_0
scoreboard players set %20 mcjava_sb_scope_0 20
#Debug: OPERATION 1 
scoreboard players operation b mcjava_sb_scope_0 = %20 mcjava_sb_scope_0
scoreboard players set %2 mcjava_sb_scope_0 2
#Debug: OPERATION 1 
scoreboard players operation c mcjava_sb_scope_0 = %2 mcjava_sb_scope_0
scoreboard players set %15 mcjava_sb_scope_0 15
#Debug: OPERATION 1 
scoreboard players operation d mcjava_sb_scope_0 = %15 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %a mcjava_sb_scope_0 = a mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %b mcjava_sb_scope_0 = b mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %c mcjava_sb_scope_0 = c mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %d mcjava_sb_scope_0 = d mcjava_sb_scope_0
tellraw @a [{"text":"a = "},{"score":{"name":"%a","objective":"mcjava_sb_scope_0"}},{"text":", b = "},{"score":{"name":"%b","objective":"mcjava_sb_scope_0"}},{"text":", c = "},{"score":{"name":"%c","objective":"mcjava_sb_scope_0"}},{"text":", d = "},{"score":{"name":"%d","objective":"mcjava_sb_scope_0"}},]
#Debug: OPERATION 2 
scoreboard players operation %a mcjava_sb_scope_0 = a mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %b mcjava_sb_scope_0 = b mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %c mcjava_sb_scope_0 = c mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %0 mcjava_sb_scope_0 = %b mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %0 mcjava_sb_scope_0 *= %c mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %a mcjava_sb_scope_0 = a mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %1 mcjava_sb_scope_0 = %0 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %1 mcjava_sb_scope_0 /= %a mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %2 mcjava_sb_scope_0 = %a mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %2 mcjava_sb_scope_0 += %1 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %b mcjava_sb_scope_0 = b mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %3 mcjava_sb_scope_0 = %2 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %3 mcjava_sb_scope_0 -= %b mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation e mcjava_sb_scope_0 = %3 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %a mcjava_sb_scope_0 = a mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %b mcjava_sb_scope_0 = b mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %4 mcjava_sb_scope_0 = %a mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %4 mcjava_sb_scope_0 += %b mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %c mcjava_sb_scope_0 = c mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %e mcjava_sb_scope_0 = e mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %5 mcjava_sb_scope_0 = %c mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %5 mcjava_sb_scope_0 -= %e mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %6 mcjava_sb_scope_0 = %4 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %6 mcjava_sb_scope_0 *= %5 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %d mcjava_sb_scope_0 = d mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %7 mcjava_sb_scope_0 = %6 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %7 mcjava_sb_scope_0 /= %d mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation f mcjava_sb_scope_0 = %7 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %e mcjava_sb_scope_0 = e mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %f mcjava_sb_scope_0 = f mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %e mcjava_sb_scope_0 = e mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %8 mcjava_sb_scope_0 = %f mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %8 mcjava_sb_scope_0 *= %e mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %a mcjava_sb_scope_0 = a mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %9 mcjava_sb_scope_0 = %8 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %9 mcjava_sb_scope_0 /= %a mcjava_sb_scope_0
#Debug: OPERATION 3 
scoreboard players operation %10 mcjava_sb_scope_0 = %e mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %10 mcjava_sb_scope_0 += %9 mcjava_sb_scope_0
scoreboard players set %1 mcjava_sb_scope_0 1
#Debug: OPERATION 3 
scoreboard players operation %11 mcjava_sb_scope_0 = %10 mcjava_sb_scope_0
#Debug: OPERATION 4 
scoreboard players operation %11 mcjava_sb_scope_0 -= %1 mcjava_sb_scope_0
#Debug: OPERATION 1 
scoreboard players operation x mcjava_sb_scope_0 = %11 mcjava_sb_scope_0
#Debug: OPERATION 2 
scoreboard players operation %e mcjava_sb_scope_0 = e mcjava_sb_scope_0
tellraw @a [{"text":"e = "},{"score":{"name":"%e","objective":"mcjava_sb_scope_0"}},]
#Debug: OPERATION 2 
scoreboard players operation %f mcjava_sb_scope_0 = f mcjava_sb_scope_0
tellraw @a [{"text":"f = "},{"score":{"name":"%f","objective":"mcjava_sb_scope_0"}},]
#Debug: OPERATION 2 
scoreboard players operation %x mcjava_sb_scope_0 = x mcjava_sb_scope_0
tellraw @a [{"text":"x = "},{"score":{"name":"%x","objective":"mcjava_sb_scope_0"}},]
#}
