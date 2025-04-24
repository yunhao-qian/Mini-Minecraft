#ifndef MINECRAFT_SCOPE_GUARD_H
#define MINECRAFT_SCOPE_GUARD_H

#include <utility>

namespace minecraft {

template<typename Callable>
class ScopeGuard
{
public:
    explicit ScopeGuard(Callable &&callable)
        : _callable{std::forward<Callable>(callable)}
    {}

    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&other) = delete;

    ~ScopeGuard() { _callable(); }

    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard &operator=(ScopeGuard &&other) = delete;

private:
    Callable _callable;
};

} // namespace minecraft

#endif // MINECRAFT_SCOPE_GUARD_H
