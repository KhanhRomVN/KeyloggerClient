#ifndef BASECOMMS_H
#define BASECOMMS_H

#include <vector>
#include <cstdint>

class BaseComms {
public:
    virtual ~BaseComms() = default;
    virtual bool Initialize() = 0;
    virtual bool SendData(const std::vector<uint8_t>& data) = 0;
    virtual void Cleanup() = 0;
    [[nodiscard]] virtual bool TestConnection() const = 0;
    virtual std::vector<uint8_t> ReceiveData() = 0;
};

#endif // BASECOMMS_H