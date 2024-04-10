if 1 then
  if 1 then
    print(101)
  else
    print(102)
  end if
else
  if 1 then
    print(201)
  else
    print(202)
  end if
end if

print 300

if 0 then
if 1 then
print(401)
else
print(402)
end if
else
  if 0 then
  print(501)
  else
  print(502)
  end if
end if

print 999

// expect: 101
// expect: 300
// expect: 502
// expect: 999