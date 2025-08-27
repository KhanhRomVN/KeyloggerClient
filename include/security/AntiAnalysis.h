#ifndef ANTIANALYSIS_H
#define ANTIANALYSIS_H

class AntiAnalysis {
public:
    static bool IsDebuggerPresent();
    static bool IsRunningInVM();
    static bool IsSandboxed();
    static void EvadeAnalysis();    
    static bool IsLowOnResources();
    static void Countermeasure();
};

#endif