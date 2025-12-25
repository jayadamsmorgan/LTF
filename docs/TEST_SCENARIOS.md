# Test scenarios

Sometimes you need more control over a test run than what CLI flags alone can express. **Scenarios** let you describe a test run in a JSON file: variables, tags, log settings, and (optionally) a preferred **test execution order**.

## Scenario file

Example scenario:

**File: `scenarios/scenario.json`**

```json
{
  "cmd": {
    "vars": {
      "var1": "somevar1",
      "var2": "somevar2"
    },
    "tags": ["tag1", "tag2", "tag3"],
    "log_level": "info"
  },
  "order": ["First test", "Second test"]
}
```

You can place scenario files anywhere, but the recommended convention is a `scenarios/` folder.

## Running a scenario

```bash
ltf test -s scenarios/scenario.json
```

### CLI overrides scenario values

Anything under `cmd` can still be overridden via CLI flags. **CLI always wins**.

```bash
ltf test -s scenarios/scenario.json -v var1:new_var1
```

In this example, `var1` will be `new_var1` even though the scenario sets it to `somevar1`.

## How ordering works

The `order` field does **not** “force-run” tests by itself.

1. LTF first collects/registers tests that match the requested `cmd.tags` (and `cmd.target` if the project is multi-target).
2. Then it tries to reorder *that selected set* according to `order`.

If a test name in `order` is not part of the selected set (wrong tags/target or not registered), it won’t run just because it’s listed.

## Properties

### `cmd.target`

* Type: `string`
* Meaning: Select the test target (only relevant for multi-target projects).

### `cmd.tags`

* Type: `string[]`
* Meaning: Tags to filter which tests are included in the run.

### `cmd.vars`

* Type: `object` (`{ "name": "value" }`)
* Meaning: Variables for the run (same idea as `-v/--vars`).

### `cmd.log_level`

* Type: `string`
* Meaning: Log verbosity.
* Allowed values: `"critical"`, `"error"`, `"warning"`, `"info"`, `"debug"`, `"trace"`.

### `cmd.no_logs`

* Type: `boolean`
* Meaning: Disable log output for the run.

### `cmd.headless`

* Type: `boolean`
* Meaning: Run without the TUI (“headless” mode).

### `cmd.ltf_lib_path`

* Type: `string`
* Meaning: Use a custom path to the LTF library.

### `order`

* Type: `string[]`
* Meaning: Desired execution order by **test name**.

## Composing and overriding scenarios

Scenarios can include another scenario and then modify parts of it. This is useful for “base scenario + small changes”.

```json
{
  "include": "path_to_other_scenario",
  "cmd": {
    "vars:append": {
      "var3": "somevar3"
    },
    "vars:remove": ["var2"],
    "tags": ["someothertag1", "someothertag2"]
  },
  "order:append": ["Third test"],
  "order:remove": ["Second test"]
}
```

### Override rules

* Use `:append` to add items to “array-like” fields.
* Use `:remove` to remove items.
* Or just redeclare the field entirely (like `cmd.tags` above).

Common patterns:

* Start with a base scenario that sets target + common vars.
* Create smaller scenarios that include it and tweak vars/tags/order.

## Examples

[See project example](./../examples/scenarios-example).
