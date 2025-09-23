# LTF Project Setup

The Test Automation Framework (LTF) is designed to be flexible, offering two primary project structures to suit your testing needs: **Single-Target** and **Multi-Target**.

## Single-Target Project (Default)

This is the standard project structure, perfect for testing a single application, service, or piece of hardware.

### 📁 Structure

The `ltf init` command creates a clean and logical directory layout:

```
your_project/
├── lib/
│   ├── your_custom_library.lua
│   └── variables.lua
├── tests/
│   ├── example_test_1.lua
│   └── example_test_2.lua
└── .ltf.json
```

*   **`tests/`**: This is the heart of your project, where all your test files (`*_test.lua`) reside.
*   **`lib/`**: This directory is designed for reusable code, helper functions, and abstractions that can be shared across multiple tests. Keeping this logic separate helps maintain clean and readable test files.
*   **`.ltf.json`**: An auto-generated file used internally by LTF to manage your project's configuration.
    > **Warning:** Do not edit `.ltf.json` manually. Use `ltf` commands to manage project settings.

### 🚀 Initialization

To create a new single-target project, run the `init` command:

```bash
ltf init your_project_name
```

This will create the `your_project_name` directory and populate it with the `lib/`, `tests/`, and `.ltf.json` files.

> **Note:** Single-Target project may be converted to Multi-Target by [Adding a Target](#adding-a-target)

---

## Multi-Target Project

If your testing involves multiple distinct environments or devices (e.g., several different embedded Linux boards, various API endpoints), the multi-target structure provides the necessary organization.

### 📁 Structure

A multi-target project extends the standard structure by organizing tests into target-specific subdirectories.

```
your_project/
├── lib/
│   └── ...
├── tests/
│   ├── common/
│   │   ├── common_test_1.lua
│   │   └── common_test_2.lua
│   ├── example_target_1/
│   │   └── target_1_tests.lua
│   └── example_target_2/
│       └── target_2_tests.lua
└── .ltf.json
```

*   **`tests/common/`**: This special directory holds tests that are generic and can be run against *any* of your defined targets. LTF will always look for tests here.
*   **`tests/<target_name>/`**: Each target you add to the project gets its own directory for tests that are specific to that target.

### 🚀 Initialization

Create a multi-target project using the `--multitarget` flag:

```bash
ltf init your_project_name --multitarget
```

This will create the base structure, including the essential `tests/common/` directory.

> **Note:** Multi-Target project may be converted to Single-Target by [Removing a Target](#adding-a-target) when you only have 1 target left

### Managing Targets

You can easily add and remove targets from your project.

#### Adding a Target

Use the `ltf target add` command to register a new target and create its test directory:

```bash
ltf target add example_target_1
```

#### Removing a Target

Use the `ltf target remove` command to unregister a target:

```bash
ltf target remove example_target_1
```

> **Note:** This command only removes the target from LTF's internal tracking. It **will not** delete the `tests/example_target_1/` directory or its contents, giving you full control over your files.

---

## A Note on File Naming

LTF does not enforce any specific naming conventions for your test or library files. You are free to name them as you see fit. LTF automatically scans the `tests/` directory (and its subdirectories) to discover all `ltf.test()` definitions.

---

# Getting Started: Your First Test

This quickstart guide will walk you through creating and running a simple test.

### 1. Initialize a New Project

First, create a new LTF project using the `init` command and navigate into the newly created directory.

```bash
ltf init my_first_ltf_project
cd my_first_ltf_project
```

### 2. Write Your Test

Next, create a new file inside the `tests/` directory (e.g., `tests/hello_world_test.lua`) and add the following code:

```lua
-- Require the LTF core library
local ltf = require("ltf")

-- Define a new test case
ltf.test("My Very First Test", function()

    ltf.log_info("The test is starting...")
    ltf.sleep(1000) -- Pause execution for 1000 milliseconds (1 second)

    -- ltf.print is an alias for ltf.log_info
    ltf.print("Hello from LTF!")

    ltf.sleep(1000)
    ltf.log_info("The test is finishing.")
end)
```

> **`ltf.test(name, body)`**
> This is the primary function for registering a new test case.
> *   The first argument is a `string` describing the test's purpose.
> *   The second argument is a `function` containing the actual test logic.

### 3. Run the Test

Save the file and run your test from the project's root directory using the `ltf test` command:

```bash
ltf test
```

You will see the LTF Terminal UI launch, execute your test, and print the log messages in real-time. The test will wait for 1 second, print "Hello from LTF!", wait another second, and then complete.

Congratulations! You've just written and successfully run your first LTF test.
