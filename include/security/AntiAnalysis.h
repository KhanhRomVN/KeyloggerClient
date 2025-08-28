#ifndef ANTIANALYSIS_H
#define ANTIANALYSIS_H

#include <string>

namespace security {

class AntiAnalysis {
public:
    static bool IsDebuggerPresent();
    static bool IsRunningInVM();
    static bool IsSandboxed();
    static void EvadeAnalysis();    
    static bool IsLowOnResources();
    static void Countermeasure();
    static void ExecuteDecoyOperations();
    static void CreateDecoyArtifacts();
    static void VMEvasionTechniques();
};

} // namespace security

#endif