
print "Lua Start"

function OnConnect(fd)
	print("OnConnect fd: " .. fd)

	--local r1, r2 = func("p111", "p222");
	
	--print("r1: "..r1 .. " r2: "..r2)

	sleep(1000)
end


function OnData(fd, msg) 
	print("OnData fd: " .. fd .. " msg: " .. msg)

	lwrite(fd, msg, string.len(msg));
end 

function OnClose(fd) 
	print("OnClose fd: " .. fd)

end 
