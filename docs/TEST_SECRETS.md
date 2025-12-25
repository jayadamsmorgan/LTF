# Test secrets

LTF supports **secrets**: named sensitive values (tokens, passwords, API keys) that are loaded from a local `.secrets` file and accessed in tests at runtime.

## Registering secrets

Register secrets at Lua **top level** using `ltf.register_secrets()`. Like [variables](./TEST_VARIABLES.md), you can call this in any file, but it’s usually best to keep it organized (for example in `lib/secrets.lua`).

```lua
local ltf = require("ltf")

ltf.register_secrets({
  "secret1",
  "secret2",
})
```

## Using secrets in a test

Use `ltf.get_secret()` to retrieve a secret by name.

```lua
local ltf = require("ltf")

ltf.test({
  name = "Example test",
  body = function()
    local secret1 = ltf.get_secret("secret1")
    local secret2 = ltf.get_secret("secret2")

    ltf.log_info("Secret value 1 is: " .. secret1)
    ltf.log_info("Secret value 2 is: " .. secret2)
  end,
})
```

## Providing secret values

Secret values are stored in a file named `.secrets` in the project root.

### Single-line secrets

```text
secret1=somesecretvalue
```

### Multiline secrets

Use triple quotes to define multiline values:

```text
secret2="""
some
other
multiline
secret
value
"""
```

Full example:

```text
secret1=somesecretvalue
secret2="""
some
other
multiline
secret
value
"""
```

## Running tests

Once the secrets are registered and `.secrets` is present:

```bash
ltf test
```

## Git safety

The `.secrets` file must **not** be committed. LTF adds it to `.gitignore` by default when you create a project with:

```bash
ltf init
```

## Validation rules

LTF validates secrets **before any tests run**:

* Every name registered via `ltf.register_secrets()` must exist in `.secrets`
* Every registered secret must have a value (including multiline values)

If any registered secret is missing (or has no value), LTF exits with an error and does not start tests.

## Example

[See project example](./../examples/secrets-example).
