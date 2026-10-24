# Ridecast CLI unit-test traceability matrix

104 CppUTest cases; each test is annotated with its requirement-style identifier.

| Test ID | Group | Test case | Objective |
|---|---|---|---|
| CLI-PARAM-001 | CliParameters | `ExtractsFirstParameter` | Parameters use one-based indexing. |
| CLI-PARAM-002 | CliParameters | `ExtractsSecondParameter` | Accessing second and subsequent parameters works. |
| CLI-PARAM-003 | CliParameters | `IgnoresWhitespaceBetweenTokens` | Leading/trailing spaces and tabs are skipped. |
| CLI-PARAM-004 | CliParameters | `RejectsNullCommandLine` | A NULL command pointer yields NULL and zero length. |
| CLI-PARAM-005 | CliParameters | `RejectsNullLengthPointer` | A NULL length pointer returns NULL. |
| CLI-PARAM-006 | CliParameters | `RejectsParameterIndexZero` | Index zero is invalid. |
| CLI-PARAM-007 | CliParameters | `RejectsParameterBeyondAvailableCount` | Missing parameter returns NULL and zero length. |
| CLI-PARAM-008 | CliParameters | `RejectsCommandWithNoArguments` | A parameter cannot be read from a command-only line. |
| CLI-PARAM-009 | CliParameters | `RejectsEmptyOrWhitespaceOnlyInput` | Empty and whitespace-only lines return NULL. |
| CLI-PARAM-010 | CliParameters | `PreservesInputString` | Original command string is not altered. |
| CLI-PARAM-011 | CliParameters | `ReturnsPointerIntoOriginalCommandLine` | Returned value is a substring, not NUL-terminated. |
| CLI-BUFFER-001 | CliOutputAccessors | `PointerIsNullBeforeInit` | Buffer pointer is NULL before initialization. |
| CLI-BUFFER-002 | CliOutputAccessors | `SizeIsZeroBeforeInit` | Buffer capacity is zero before initialization. |
| CLI-BUFFER-003 | CliOutputAccessors | `PointerEqualsConfiguredBuffer` | API exposes the exact application-owned buffer. |
| CLI-BUFFER-004 | CliOutputAccessors | `SizeEqualsConfiguredCapacity` | Reported capacity includes NUL terminator. |
| CLI-BUFFER-005 | CliOutputAccessors | `AccessorsResetAfterDeinit` | Getter results reset after deinitialization. |
| CLI-PARAM-012 | CliParameters | `RejectsHighIndexWhenNoParametersExist` | No available parameters when skipping multiple indexes. |
| CLI-REG-001 | CliRegistration | `RejectsRegistrationBeforeInit` | Commands cannot be registered before initialization. |
| CLI-REG-002 | CliRegistration | `RejectsNullCommand` | NULL descriptor shall be rejected. |
| CLI-REG-003 | CliRegistration | `RejectsMissingCommandName` | Empty or NULL command name shall be rejected. |
| CLI-REG-004 | CliRegistration | `RejectsNullHelpOrCallback` | Command description and callback are required. |
| CLI-REG-005 | CliRegistration | `RejectsInvalidCommandNameCharacters` | Command names cannot contain whitespace or control characters. |
| CLI-REG-006 | CliRegistration | `RejectsReservedHelpCommand` | The built-in help name is reserved. |
| CLI-REG-007 | CliRegistration | `RejectsDuplicateName` | Duplicate names are rejected independent of pointer identity. |
| CLI-REG-008 | CliRegistration | `ValidatesExpectedParameterCount` | -1 is accepted for variable parameters; -2 is invalid. |
| CLI-REG-009 | CliRegistration | `RegistryCapacityIsBounded` | The fixed table rejects entries past its capacity. |
| CLI-REG-010 | CliRegistration | `CommandNamesAreCaseSensitive` | Different case constitutes a different command name. |
| CLI-EXEC-001 | CliExecution | `RejectsExecutionBeforeInitialization` | Command execution requires initialization. |
| CLI-EXEC-002 | CliExecution | `RejectsNullCommandLine` | NULL input is invalid. |
| CLI-EXEC-003 | CliExecution | `IgnoresBlankAndWhitespaceOnlyLines` | Blank input does not produce transport traffic. |
| CLI-EXEC-004 | CliExecution | `ExecutesCommandAndWritesOutput` | Executing a registered command calls its callback. |
| CLI-EXEC-005 | CliExecution | `SkipsLeadingWhitespace` | Leading whitespace is accepted and command line forwarded. |
| CLI-EXEC-006 | CliExecution | `PrefixIsNotAcceptedAsCommand` | Command matching shall use complete tokens. |
| CLI-EXEC-007 | CliExecution | `UnknownCommandReportsFriendlyError` | Unknown commands report a message without processing error. |
| CLI-EXEC-008 | CliExecution | `RejectsWrongParameterCount` | Fixed parameter count prevents callback invocation. |
| CLI-EXEC-009 | CliExecution | `AcceptsCorrectParameterCount` | Exact parameter counts handle multiple separators. |
| CLI-EXEC-010 | CliExecution | `AllowsVariableParameterCount` | Variable argument count bypasses validation. |
| CLI-EXEC-011 | CliExecution | `HelpIncludesBuiltInAndRegisteredCommands` | Built-in help enumerates registered commands. |
| CLI-EXEC-012 | CliExecution | `HelpRejectsArguments` | Help refuses extra parameters. |
| CLI-EXEC-013 | CliExecution | `HelpAddsNewlineWhenMissing` | Help emits a newline for a help string lacking one. |
| CLI-EXEC-014 | CliExecution | `HelpHandlesEmptyHelpDescription` | Empty help strings are handled without underflow. |
| CLI-EXEC-015 | CliExecution | `SupportsMultiChunkOutput` | A command can produce multiple output chunks. |
| CLI-EXEC-016 | CliExecution | `RejectsNoProgressWhenMoreOutputRequested` | Repeated callback requests with empty output are rejected. |
| CLI-EXEC-017 | CliExecution | `LimitsMaximumOutputChunks` | Infinite more-output callbacks are bounded. |
| CLI-EXEC-018 | CliExecution | `PermitsSuccessfulEmptyOutput` | A callback can finish without writing data. |
| CLI-EXEC-019 | CliExecution | `SafelyTerminatesCallbackOutput` | The final output byte is forced to NUL. |
| CLI-EXEC-020 | CliExecution | `PropagatesWriteFailure` | Command write failures are propagated. |
| CLI-EXEC-021 | CliExecution | `PropagatesHelpWriteFailure` | Help transport failures propagate immediately. |
| CLI-EXEC-022 | CliExecution | `ForwardsCommandContext` | Each registration stores a callback-specific context. |
| CLI-EXEC-023 | CliExecution | `PropagatesHelpIntroductionFailure` | Failure writing the help introduction is returned. |
| CLI-EXEC-024 | CliExecution | `PropagatesHelpNewlineFailure` | Failure writing an appended newline is returned. |
| CLI-INIT-001 | CliInitialization | `RejectsNullConfiguration` | Reject a NULL configuration without dereference. |
| CLI-INIT-002 | CliInitialization | `InitializesResourcesAndWritesPrompt` | Prompt is emitted once on successful initialization. |
| CLI-INIT-003 | CliInitialization | `RejectsDuplicateInitialization` | Already initialized state is reported. |
| CLI-INIT-004 | CliInitialization | `RejectsMissingWriteCallback` | A transport write callback is mandatory. |
| CLI-INIT-005 | CliInitialization | `RejectsMissingCommandBuffer` | A command buffer pointer is mandatory. |
| CLI-INIT-006 | CliInitialization | `RejectsCommandBufferSizesZeroAndOne` | Buffer capacity must reserve a NUL byte. |
| CLI-INIT-007 | CliInitialization | `RejectsMissingOutputBuffer` | An output buffer pointer is mandatory. |
| CLI-INIT-008 | CliInitialization | `RejectsOutputBufferSizesZeroAndOne` | Output capacity must reserve a NUL byte. |
| CLI-INIT-009 | CliInitialization | `RejectsReadCallbackWithoutReadBuffer` | Polling requires a valid read buffer. |
| CLI-INIT-010 | CliInitialization | `RejectsZeroReadBufferCapacity` | Zero read capacity is invalid when read callback exists. |
| CLI-INIT-011 | CliInitialization | `RejectsReadBufferCapacityAboveInt32Max` | Read callback's int32 return limits its capacity. |
| CLI-INIT-012 | CliInitialization | `AllowsPushInputMode` | Push-input mode requires no read callback/buffer. |
| CLI-INIT-013 | CliInitialization | `InitializesWithoutPrompt` | Prompt is optional. |
| CLI-INIT-014 | CliInitialization | `RejectsSameCommandAndOutputBuffer` | The same buffer cannot back input and output. |
| CLI-INIT-015 | CliInitialization | `RejectsReadBufferAliasingCommandBuffer` | An RX buffer cannot alias command storage. |
| CLI-INIT-016 | CliInitialization | `RejectsReadBufferAliasingOutputBuffer` | An RX buffer cannot alias output storage. |
| CLI-INIT-017 | CliInitialization | `PromptFailureAllowsRetry` | Failed initial prompt write rolls back module state. |
| CLI-INIT-018 | CliInitialization | `InitializesCommandAndOutputStrings` | Application-owned buffers are cleared on initialization. |
| CLI-DEINIT-001 | CliDeinitialization | `RejectsDeinitBeforeInitialization` | Calling deinit without init is deterministic. |
| CLI-DEINIT-002 | CliDeinitialization | `ClearsStateAndAllowsReinitialization` | Deinitialization clears the CLI state. |
| CLI-DEINIT-003 | CliDeinitialization | `RemovesRegisteredCommands` | All registered command references are removed. |
| CLI-DEINIT-004 | CliDeinitialization | `DoesNotDestroyApplicationOwnedBuffers` | Deinit does not free or overwrite owned buffers. |
| CLI-DEINIT-005 | CliDeinitialization | `DoubleDeinitReturnsNotInitialized` | Repeated deinit cannot succeed twice. |
| CLI-INPUT-001 | CliInput | `RejectsInputBeforeInitialization` | Processing data before initialization fails. |
| CLI-INPUT-002 | CliInput | `RejectsNullInputWhenLengthIsNonzero` | NULL pointer with nonzero length is invalid. |
| CLI-INPUT-003 | CliInput | `AcceptsNullInputWhenLengthIsZero` | A NULL pointer with zero length is a no-op. |
| CLI-INPUT-004 | CliInput | `AccumulatesPartialInput` | Command is accumulated across separate RX calls. |
| CLI-INPUT-005 | CliInput | `ExecutesOnCarriageReturn` | A carriage return dispatches one command. |
| CLI-INPUT-006 | CliInput | `ExecutesOnLineFeed` | A line feed dispatches one command. |
| CLI-INPUT-007 | CliInput | `SuppressesSplitCrLfPair` | CRLF is one terminator, including across RX chunks. |
| CLI-INPUT-008 | CliInput | `SuppressesSplitLfCrPair` | LFCR is one terminator, including across RX chunks. |
| CLI-INPUT-009 | CliInput | `ProcessesMultipleCommandsInOneInputFrame` | Two full lines in a single frame execute twice. |
| CLI-INPUT-010 | CliInput | `HandlesBackspace` | Backspace removes exactly one input character. |
| CLI-INPUT-011 | CliInput | `HandlesAsciiDelete` | ASCII DEL has the same editing effect as backspace. |
| CLI-INPUT-012 | CliInput | `IgnoresBackspaceAtStartOfLine` | Backspace at index zero does not underflow. |
| CLI-INPUT-013 | CliInput | `EchoesPrintableCharactersAndBackspace` | Echo echoes printable bytes and erases one byte. |
| CLI-INPUT-014 | CliInput | `DoesNotEchoWhenDisabled` | Disabled echo never sends typed characters or EOL. |
| CLI-INPUT-015 | CliInput | `IgnoresUnsupportedControlBytes` | Unsupported controls do not enter the command buffer. |
| CLI-INPUT-016 | CliInput | `AcceptsCommandAtMaximumPermittedLength` | Exactly capacity - 1 printable bytes fit. |
| CLI-INPUT-017 | CliInput | `RejectsOversizedLineWithoutRunningTruncatedPrefix` | Any additional printable byte discards whole line. |
| CLI-INPUT-018 | CliInput | `RecoversAfterDiscardingOverlongLine` | Oversized-line state is retained between reads. |
| CLI-INPUT-019 | CliInput | `PropagatesEchoWriteFailure` | Input processing propagates transport errors. |
| CLI-INPUT-020 | CliInput | `KeepsFirstFailureAcrossInputFrame` | The first error wins when processing multiple bytes. |
| CLI-INPUT-021 | CliInput | `PropagatesPromptFailureAfterInput` | Prompt output failure must be returned to the caller. |
| CLI-INPUT-022 | CliInput | `ClearsInputStateWhenCommandExecutionFails` | Next line remains valid after command callback error. |
| CLI-CONSOLE-001 | CliConsoleProcessing | `RejectsBeforeInitialization` | Console processing requires initialization. |
| CLI-CONSOLE-002 | CliConsoleProcessing | `RejectsMissingReadCallback` | Push-only mode cannot poll a missing read callback. |
| CLI-CONSOLE-003 | CliConsoleProcessing | `ReturnsOkWhenReadHasNoData` | Zero received bytes are a successful no-op. |
| CLI-CONSOLE-004 | CliConsoleProcessing | `ConvertsNegativeReadToRxFailed` | Negative transport read result maps to RX_FAILED. |
| CLI-CONSOLE-005 | CliConsoleProcessing | `RejectsReportedLengthGreaterThanReadBuffer` | Oversized reported RX length is rejected. |
| CLI-CONSOLE-006 | CliConsoleProcessing | `ReadsAndProcessesCommand` | A valid RX frame dispatches the command. |
| CLI-CONSOLE-007 | CliConsoleProcessing | `ReassemblesCommandAcrossTransportReads` | RX frames larger than read buffer are accumulated. |
| CLI-CONSOLE-008 | CliConsoleProcessing | `PropagatesReceivedCommandFailure` | Processing errors propagate through the console API. |
