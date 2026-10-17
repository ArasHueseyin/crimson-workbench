#pragma once
namespace crimson::marketplace::mount_protection {
enum class InstallResult { pin_failed, initialize_failed, create_failed, activation_failed, ready };

template<class Native>
InstallResult install_hook(Native& native) {
    if (!native.pin_module()) return InstallResult::pin_failed;
    if (!native.initialize()) return InstallResult::initialize_failed;
    if (!native.create()) return InstallResult::create_failed;
    // An activation attempt can already have entered a callback. Its published
    // trampoline and the pinned module must remain alive even on reported failure.
    if (!native.enable()) return InstallResult::activation_failed;
    return InstallResult::ready;
}
} // namespace crimson::marketplace::mount_protection
