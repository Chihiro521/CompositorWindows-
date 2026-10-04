# Current checkpoint

Windows preview **0.1.4** is based on Mac Compositor 1.0.4 at `a19db9011282399785dc18efcfded904627bdcc2`. The public repository is https://github.com/IAmTheBlurr/CompositorWindows. The complete existing Windows commit history is retained.

The current interface has a vector tool rail, custom dark controls, Inter typography, compact adjustment panels, a themed project picker, and Mac-informed layer rows. Native Windows title bars are the default; View → Appearance → Mac-style title bar enables the saved optional appearance. This preference only changes window chrome.

The localized source variant keeps product version **0.1.4** and adds English / 简体中文 switching under View → Language. It stores the choice, follows Windows UI language on first launch, and translates interface controls without changing project content. Its local candidate package is under `dist/`; it includes the MSI, portable ZIP, matching source ZIP, demo ZIP, manifest, and checksums. It has not been published.

The initial publication consolidates user/build documentation, adds contributor guidance, and removes 712 generated or obsolete tracked files from the current tree. Local raw evidence and archived material are private to the development workspace. Historical commits remain intact. Known limitations are in KNOWN-ISSUES.md; actual checks are summarized in VALIDATION.md.

The 0.1.4 package identifies source commit `be63afd9ac9ed1b3fdc25c3692da17be456548ee`, with 490 corresponding source files and `sourceDirty: false`. The MSI and portable archive include the runtime, model, licenses, and dependency sources. The matching source ZIP, demo ZIP, hashes, and manifest accompany the release. Local MSI upgrade and installed runtime health pass. Default installation is `%LOCALAPPDATA%\Programs\CompositorWindows\Compositor.exe`.

Maintenance intent: stay aligned with Robbie's core features, editing behavior, design, and project format. Newer upstream releases have not been integrated. Version alignment and automated upstream handling remain future work, not active claims. Improvements that fit the foundation are welcome.

Keep this as the single current checkpoint. Follow AGENTS.md for documentation boundaries. No implementation backlog is implied by historical reports or old tests; start with the user's current request.
