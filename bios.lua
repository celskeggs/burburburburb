-- core
function debug(x)
	sysbusctl(127, x)
end
-- driver framework
local drivers = {}
do -- initialization
	local resources = {}
	local driver_table = {}
	local acquirement = {}
	function drivers.add_resource(kind, name, resource)
		assert(resource, "resource is nil")
		if not resources[kind] then resources[kind] = {} end
		assert(resources[kind][name] == nil, "resource already exists")
		resources[kind][name] = resource
		debug("resource made available: " .. kind .. "/" .. name)
		if driver_table[kind] then
			driver_table[kind].onadd(kind, name, resource)
		end
	end
	function drivers.remove_resource(kind, name)
		assert(resources[kind] and resources[kind][name], "resource does not exist")
		local resource = resources[kind][name]
		resources[kind][name] = nil
		debug("resource made unavailable: " .. kind .. "/" .. name)
		if driver_table[kind] then
			driver_table[kind].onremove(kind, name, resource)
		end
		if acquirement[kind] and acquirement[kind][name] then
			acquirement[kind][name] = nil
		end
	end
	function drivers.add_driver(kind, name, onadd, onremove)
		assert(driver_table[kind] == nil, "driver already exists")
		driver_table[kind] = {kind=kind, name=name, onadd=onadd, onremove=onremove}
		debug("driver registered: " .. kind .. "/" .. name)
		for k,v in pairs(resources[kind]) do
			onadd(kind, k, v)
		end
	end
	function drivers.remove_driver(kind, name)
		assert(driver_table[kind], "driver does not exist")
		for k,v in pairs(resources[kind]) do
			driver_table[kind].onremove(kind, k, v)
		end
		driver_table[kind] = nil
		debug("driver unregistered: " .. kind .. "/" .. name)
	end
	function drivers.list(kind)
		local out = {}
		if resources[kind] then
			for k,v in pairs(resources[kind]) do
				table.insert(out, k)
			end
		end
		return out
	end
	function drivers.acquire_any(kind)
		if resources[kind] then
			for name,v in pairs(resources[kind]) do
				return name, drivers.acquire(kind, name)
			end
		end
	end
	function drivers.acquire(kind, name)
		assert(driver_table[kind] == nil, "cannot acquire a driver-handled resource")
		assert(resources[kind] ~= nil and resources[kind][name] ~= nil, "resource does not exist")
		if acquirement[kind] == nil then acquirement[kind] = {} end
		assert(acquirement[kind][name] == nil, "resource already acquired")
		acquirement[kind][name] = true
		return resources[kind][name]
	end
	function drivers.release(kind, name)
		assert(acquirement[kind] == nil or acquirement[kind][name] == nil, "resource not acquired")
		acquirement[kind][name] = nil
	end
end
-- sysbus module
local sysbus = {}
do -- initialization
	debug("checking sysbus...")
	out = sysbusctl(0)
	if out == 0xFFFF then
		error("no hardware attached!")
	elseif out ~= 0xCA72 then
		error("unrecognized sysbus version!")
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
	debug("configuring sysbus...")
	local enumid = 0
	if sysbusctl_boolean(1) then
		repeat
			sysbusctl(6, enumid)
			assert(sysbusctl(5) == enumid)
			typeid = sysbusctl(4)
			drivers.add_resource("sysbus.raw", typeid .. "." .. enumid, {typeid=typeid, busid=enumid})
			enumid = enumid + 1
		until not sysbusctl_boolean(2)
	end
	debug("total sysbus devices: " .. enumid)
end
-- basic drivers
do -- sysbus.raw.dispatch
	local sysbus_naming = {}
	sysbus_naming[0x0000] = "null"
	sysbus_naming[0x9C92] = "serial_in"
	sysbus_naming[0xCE43] = "serial_out"
	sysbus_naming[0xEA17] = "zero"
	drivers.add_driver("sysbus.raw", "sysbus.raw.dispatch", function(kind, name, resource)
		local skind = sysbus_naming[resource.typeid]
		if skind then
			drivers.add_resource("sysbus." .. skind, tostring(resource.busid), resource.busid)
		else
			debug("unrecognized sysbus typeid " .. resource.typeid)
		end
	end, function(kind, name, resource)
		local skind = sysbus_naming[resource.typeid]
		if skind then
			drivers.remove_resource("sysbus." .. skind, tostring(resource.busid))
		end
	end)
end
do -- sysbus.serial.out
	drivers.add_driver("sysbus.serial_out", "sysbus.serial.out", function(kind, name, resource)
		assert(type(resource) == "number")
		local oid = resource + 256
		local obj = {}
		function obj.write(b)
			if type(b) == "string" then
				for i,v in ipairs({string.byte(b,1,#b)}) do
					sysbusctl(oid, v)
				end
			else
				assert(type(b) == "number")
				sysbusctl(oid, b)
			end
		end
		drivers.add_resource("serial.out", "sysbus." .. name, obj)
	end, function(kind, name, resource)
		drivers.remove_resource("serial.out", "sysbus." .. name)
	end)
end
-- main code
debug("starting main code...")
local serial_name, serial_port = drivers.acquire_any("serial.out")
if not serial_port then
	debug("no serial port found")
else
	debug("serial port found: " .. serial_name)
	serial_port.write("Hello, World!\n")
end
--[[function write(x)
	for k,v in ipairs({string.byte(x,1,#x)}) do
		sysbusctl(257, v)
	end
end
write("Hello, World!\n")
function getchar()
	debug("got " .. sysbusctl(258, 0))
end
getchar()]]

