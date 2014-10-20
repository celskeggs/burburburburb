-- TODO: review print
local env_coroutine = {create=coroutine.create, resume=coroutine.resume, running=coroutine.running, status=coroutine.status, wrap=coroutine.wrap, yield=coroutine.yield}
local env_string = {byte=string.byte, char=string.char, find=string.find, format=string.format, gmatch=string.gmatch, gsub=string.gsub, len=string.len, lower=string.lower, match=string.match, rep=string.rep, reverse=string.reverse, sub=string.sub, upper=string.upper}
local env_table = {insert=table.insert, maxn=table.maxn, remove=table.remove, sort=table.sort}
local env_math = {abs=math.abs, acos=math.acos, asin=math.asin, atan=math.atan, atan2=math.atan2, ceil=math.ceil, cos=math.cos, cosh=math.cosh, deg=math.deg, exp=math.exp, floor=math.floor, fmod=math.fmod, frexp=math.frexp, huge=math.huge, ldexp=math.ldexp, log=math.log, log10=math.log10, max=math.max, min=math.min, modf=math.modf, pi=math.pi, pow=math.pow, rad=math.rad, random=math.random, sin=math.sin, sinh=math.sinh, sqrt=math.sqrt, tan=math.tan, tanh=math.tanh}
local env = {pairs=pairs, assert=assert, error=error, ipairs=ipairs, next=next, pcall=pcall, select=select, tonumber=tonumber, tostring=tostring, type=type, unpack=unpack, _VERSION=_VERSION, xpcall=xpcall, coroutine=env_coroutine, string=env_string, table=env_table, math=env_math, sysbusctl=sysbusctl}
env._G = env
local untrusted, err = loadfile("bios.lua", "t", env)
if untrusted then
	success, err = pcall(untrusted)
	if success then
		-- the bios may return either "poweroff" or "reboot"
	else
		print("Could not run bios.lua:")
		print(err)
	end
else
	print("Could not load bios.lua:")
	print(err)
end

