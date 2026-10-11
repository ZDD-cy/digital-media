#include "IndigoSession.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndigoFlowTest,"Indigo.Whitebox.All36Flows",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndigoFlowTest::RunTest(const FString& Parameters) {
    for(int32 L=0;L<3;++L) for(int32 R=0;R<3;++R) for(int32 D=1;D<=4;++D) {
        FIndigoSession S;
        auto A=[&](const TCHAR* Action,int32 V=-1){TestTrue(FString::Printf(TEXT("%d%d/%d %s"),L,R,D,Action),S.Act(Action,V));};
        auto Done=[&](){S.Advance(100);};
        TestFalse(TEXT("Cannot skip opening"),S.Act("next")); A(TEXT("old")); A(TEXT("letter")); A(TEXT("next"));
        A(TEXT("left"),L); A(TEXT("right"),R); A(TEXT("swap")); TestEqual(TEXT("Left swapped"),S.LeftPattern,R); A(TEXT("swap"));
        TestFalse(TEXT("Invalid pattern rejected"),S.Act("left",3)); A(TEXT("next"));
        A(TEXT("test")); A(TEXT("test")); A(TEXT("test")); TestFalse(TEXT("Rapid test click rejected"),S.Act("test")); Done(); A(TEXT("next")); A(TEXT("heat"));
        TestFalse(TEXT("Heating double click rejected"),S.Act("heat")); Done();
        A(TEXT("test")); A(TEXT("test")); A(TEXT("test")); Done(); A(TEXT("next"));
        for(int I=0;I<3;++I){A(TEXT("fold"));Done();}
        TestFalse(TEXT("Must put cloth first"),S.Act("lid")); A(TEXT("cloth"));Done();A(TEXT("lid"));Done();A(TEXT("clipL"));A(TEXT("clipR"));Done();
        TestFalse(TEXT("No zero-depth finish"),S.Act("finish")); TestFalse(TEXT("Ready heat rejected"),S.Act("heat")); TestEqual(TEXT("One fuel used"),S.Fuel,4);
        for(int I=1;I<=D;++I) {
            A(TEXT("dye"));TestFalse(TEXT("No double dye"),S.Act("dye"));TestFalse(TEXT("No animation skip"),S.Act("finish"));Done();
            TestEqual(TEXT("One depth per stage"),S.DyeDepth,I); TestFalse(TEXT("Cold dye rejected"),S.Act("dye"));
            if(I<D){A(TEXT("heat"));Done();}
        }
        TestEqual(TEXT("Fuel accounting"),S.Fuel,5-D);
        if(D==4) TestFalse(TEXT("No fifth-stage heating"),S.Act("heat"));
        A(TEXT("finish"));A(TEXT("wash"));Done();A(TEXT("wash"));Done();A(TEXT("openL"));TestFalse(TEXT("Both clips required"),S.Act("next"));A(TEXT("openR"));A(TEXT("next"));Done();
        TestTrue(TEXT("Reveal reached"),S.Step==EIndigoStep::Reveal);A(TEXT("next"));A(TEXT("next"));A(TEXT("next"));Done();Done();Done();Done();
        TestTrue(TEXT("End reached"),S.Step==EIndigoStep::End);A(TEXT("review"));A(TEXT("next"));
        TestEqual(TEXT("Left preserved"),S.LeftPattern,L);TestEqual(TEXT("Right preserved"),S.RightPattern,R);TestEqual(TEXT("Depth preserved"),S.DyeDepth,D);
        A(TEXT("restart"));TestEqual(TEXT("Reset fuel"),S.Fuel,5);TestEqual(TEXT("Reset depth"),S.DyeDepth,0);TestEqual(TEXT("Reset left"),S.LeftPattern,-1);TestEqual(TEXT("Reset right"),S.RightPattern,-1);
        TestFalse(TEXT("Reset clips"),S.ClipLeft||S.ClipRight||S.OpenLeft||S.OpenRight);TestFalse(TEXT("Reset temperature"),S.Ready);TestFalse(TEXT("No pending animation"),S.Busy());
    }
    return true;
}
#endif
