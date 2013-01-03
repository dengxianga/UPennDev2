serialization = {}

local numlua_support = false
if pcall(require, 'numlua') then
  numlua_support = true
end

function serialization.serialize(o)
  local str = 'nil'

  -- serialize object
  if (type(o) == 'boolean') then
    str = tostring(o)
  elseif (type(o) == 'number') then
    str = tostring(o) 
    if (str == 'inf') then
      str = 'math.huge'
    elseif (str == '-inf') then 
      str = '-math.huge'
    end
  elseif (type(o) == 'string') then
    str = string.format('%q',o)
  elseif (type(o) == 'table') then
    local entries = {}
    for k,v in pairs(o) do
      entries[#entries + 1] = string.format('[%s]=%s',
        serialization.serialize(k),
        serialization.serialize(v)
      )
    end
    str = '{'..table.concat(entries, ',')..'}'
  elseif (type(o) == 'userdata') then
    if numlua_support and (numlua.type(o) == 'matrix') then
      str = string.format('matrix.fromtable(%s)',
        serialization.serialize(matrix.totable(o))
      )
    elseif numlua_support and (numlua.type(o) == 'complex') then
       str = string.format('complex(%f, %f)', o:real(), o:imag())
    end
  end

  -- set object metatable if __mtstring field is defined
  local mt = getmetatable(o)
  if (mt and mt.__mtstring) then
    str = 'setmetatable('..str..','..mt.__mtstring..')'
  end

  return str
end

function serialization.deserialize(s)
  --local x = assert(loadstring("return "..s))();
  local x = loadstring("return "..s)()
  if (not x) then
    print(string.format("Could not deserialize: %s",s))
  end
  return x
end

return serialization
