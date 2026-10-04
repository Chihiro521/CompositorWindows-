# Validation

Results below describe actual local checkpoints, not Mac equivalence or broad hardware certification. Raw logs and dumps are retained locally; generated evidence is excluded from this repository.

| Checkpoint | Result |
| --- | --- |
| Publication preview 0.1.4 | Release application builds from the staged source export using the prepared dependency cache. 24 selected CTest checks, the manual-update policy check, native window-control probe, and 18-step editing workflow pass. |
| Preview 0.1.4 downloads | Six portable deployment checks pass, including hashes, matching source, manual-update distribution, cleaned-PATH health, native editing, and Unicode project reopen/export. Local MSI upgrade and installed cleaned-PATH health exit 0; the installed executable matches the package. |
| Preview 0.1.3 window appearance | Release build; 43 affected UI checks pass. Native title bar switching, persistence, dialogs, and canvas probes pass at 100% and 150%. |
| Simplified Chinese localized source, product version 0.1.4 | 606 Qt Linguist translations; XML, context/source duplicate, and placeholder checks pass. Release build succeeds with Qt 6.8.3, VS Build Tools 2026 using MSVC v143 14.44, and Windows SDK 10.0.26100.0. Eleven selected CTest checks pass, covering first-run Windows language detection, saved-language precedence, runtime switching, project tabs, and project chrome. The 19-step native WARP workflow passes at 100% and 150% scaling; Chinese menus, controls, palette, and adjustment panels were visually reviewed at both scales. An early standalone launch lacked the Qt runtime/plugin search paths and exited with 0xc0000135; after setting the test environment, the affected checks passed. The local candidate under `dist/` contains the MSI, portable ZIP, matching source ZIP, demo ZIP, manifest, hashes, and Qt base Chinese dialog catalog. All 13 packaging checks pass, including clean-PATH health, portable and installed workflows, MSI repair, uninstall, payload hashes, and preservation of an existing project; Explorer menu interaction itself was not manually exercised. Product version remains 0.1.4; the candidate was not published. |
| Preview 0.1.3 installed application | MSI upgrade and runtime health exit 0. The 18-check native editing workflow passes with a cleaned PATH. Four package hashes and 474 source entries match the packaged snapshot. |
| Preview 0.1.2 visual update | 75 focused checks pass; 33 affected panel checks pass after final refinement; seven native visibility accessibility checks pass. |
| Last full normal regression | 926/932 pass. The six retained discrepancies are described in KNOWN-ISSUES.md. |
| Last full ASAN regression | 925/932 pass. Six discrepancies plus an intermittent process-exit timeout remain. |
| Independent Levels cancellation oracle | Pass in normal and ASAN; the original failing frozen comparison remains unchanged. |
| Preview 0.1.0 packaging | 16/16 deployment checks, including payload/source hashes, runtime health, editing/save/export, MSI install/repair/upgrade/downgrade rejection/uninstall, and project preservation. |
| Source rebuild | Offline bootstrap, configure, Release application build, and deployed runtime health pass on the development host. This does not establish byte-identical binaries. |
| Brush performance | Two runs each passed 6/10 existing dispatch gates. Large-brush latency and background-load caveats remain disclosed. |
| Foreground removal | Eight cases executed; seven masks and one no-subject result. Published-cutout agreement passes both modes; fine-edge and transparent-object limitations remain. |

Native workflow checks cover import, layers/masks, transformation, selections, painting, undo/redo, preview cancel/apply, save/reopen, export, and shutdown. Screenshots combine actual Qt rendering with the Direct3D canvas readback where normal window capture cannot include it.

Original assertions and test sources remain available. Optional tests that need external image corpora, historical audit inventories, or a Mac reference capture require those inputs. Their absence must not be represented as passing verification. Fresh Windows and Mac comparisons have not been performed.
