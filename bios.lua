-- TODO
local old_sysbusctl = sysbusctl
function debug(x)
	old_sysbusctl(127, x)
end
debug("starting...")
function sysbusctl(x, ...)
	out = old_sysbusctl(x, ...)
	--debug("sysbus cmd " .. x .. " -> " .. out)
	return out
end
function sysbus_check()
	debug("checking sysbus...")
	out = sysbusctl(0)
	if out == 0xFFFF then
		error("no hardware attached!")
	elseif out ~= 0xCA72 then
		error("unrecognized sysbus version!")
	end
	debug("passed sysbus check.")
end
function sysbusctl_boolean(cmd)
	out = sysbusctl(cmd)
	if out == 0xFFFF then
		error("sysbus command " .. out .. " error'd!")
	elseif out == 0x0000 then
		return false
	elseif out == 0x0001 then
		return true
	else
		error("sysbus command " .. out .. " gave a bad result: " .. out .. "!")
	end
end
sysbus_check()
function sysbus_enumerate()
	debug("enumerating sysbus...")
	local enumid = 0
	if sysbusctl_boolean(1) then
		repeat
			debug("enumid " .. enumid .. " typeid " .. sysbusctl(4) .. " busid " .. sysbusctl(5))
			enumid = enumid + 1
		until not sysbusctl_boolean(2)
	end
	debug("total sysbus devices: " .. enumid)
end
function sysbus_configure()
	debug("configuring sysbus...")
	if sysbusctl_boolean(1) then
		repeat
			local busid = sysbusctl(5)
			if busid ~= 255 then
				sysbusctl(6, 255)
			end
			assert(sysbusctl(5) == 255)
		until not sysbusctl_boolean(2)
	end
	local enumid = 0
	if sysbusctl_boolean(1) then
		repeat
			local busid = sysbusctl(5)
			sysbusctl(6, enumid)
			assert(sysbusctl(5) == enumid)
			enumid = enumid + 1
		until not sysbusctl_boolean(2)
	end
	debug("total sysbus devices: " .. enumid)
end
sysbus_enumerate()
sysbus_configure()
sysbus_enumerate()
function write(x)
	for k,v in ipairs({string.byte(x,1,#x)}) do
		sysbusctl(257, v)
	end
end
write("Hello, World!\n")
function getchar()
	debug("got " .. sysbusctl(258, 0))
end
getchar()
debug("halting system")

