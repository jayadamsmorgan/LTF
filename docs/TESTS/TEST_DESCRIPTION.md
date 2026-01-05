# LTF Test Description

Every LTF Test can also have an optional description field if you would like to give more context to the test besides the test name.

```lua
local ltf = require("ltf")

-- This test has a single-line description
ltf.test({
    name = "Example Test 1",
    description = "This is just an example test",
    body = function()
        -- Some very important test logic goes here
    end,
})

-- This test has a multiline description
ltf.test({
    name = "Example Test 2",
    description = [[
    This is an example of multiline
    test description.
    You can describe everything here.
    ]],
    body = function()
        -- Some very important test logic goes here
    end,
})

-- This test has no description
ltf.test({
    name = "Example Test 3",
    body = function()
        -- Some very important test logic goes here
    end,
})
```
