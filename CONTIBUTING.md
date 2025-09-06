## Git Commit Guidelines

To maintain a **clean, consistent, and meaningful commit history**, we follow the **Conventional Commits** standard. This helps with changelogs, code reviews, and collaboration across teams.

### Commit Message Format
```bash
<type>(<scope>): <short description>
```
- **type**: Describes the purpose of the commit (see table below).  
- **scope** *(optional but recommended)*: The area/module affected (e.g., `dispatcher`, `uart_hal`).  
- **short description**: A concise summary of the change in present tense.  



### Common Commit Types

| Type       | Description |
|-----------|-------------|
| **init**    | Initial project setup or repository scaffolding |
| **feat**    | New feature or functionality |
| **fix**     | Bug fix or issue resolution |
| **ci**      | Changes to CI/CD scripts or configuration |
| **docs**    | Documentation changes or updates |
| **style**   | Formatting, indentation, or code style changes (no logic change) |
| **refactor**| Code restructuring or cleanup (no feature or fix) |
| **perf**    | Performance improvements |
| **test**    | Adding or updating unit/integration tests |
| **chore**   | Miscellaneous tasks (build scripts, dependencies, tooling) |
| **update**  | Minor updates, version bumps, or non-functional changes |


**Example:**
* init: set up project folder structure
* feat(middleware): add AES-GCM encryption for UART
* fix(wifi_hal): resolve TCP disconnect issue
* ci: integrate GitHub Actions for automated build
* docs: add data flow diagrams and README updates
* test(integration): add phone-to-MCU end-to-end test
* refactor(dispatcher): simplify routing logic
* update: upgrade ESP-IDF SDK to v5.1

---

### Best Practices

1. **Use present tense** in commit messages (e.g., `fix bug`, not `fixed bug`).  
2. **Keep the description concise** — one line is ideal.  
3. **Scope is recommended** to clarify which module/component the change affects.  
4. **Link issues** if applicable:  