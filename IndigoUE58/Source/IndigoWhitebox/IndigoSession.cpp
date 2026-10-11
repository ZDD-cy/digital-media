#include "IndigoSession.h"

bool FIndigoSession::Act(const FString& A, int32 V) {
    if(Busy()) return false;
    using E=EIndigoStep;
    if(Step==E::Receive) {
        if(A=="old") { SawOld=true; return true; }
        if(A=="letter") { SawLetter=true; return true; }
        if(A=="next" && SawOld && SawLetter) { Enter(E::Pattern); return true; }
    } else if(Step==E::Pattern) {
        if((A=="left" || A=="right") && V>=-1 && V<=2) { (A=="left"?LeftPattern:RightPattern)=V; return true; }
        if(A=="swap") { Swap(LeftPattern,RightPattern); return true; }
        if(A=="next" && LeftPattern>=0 && RightPattern>=0) { Enter(E::Test); return true; }
    } else if(Step==E::Test || Step==E::Retest) {
        if(A=="test" && TestPhase<3) { ++TestPhase; if(TestPhase==3) Animate("test",3.3f); return true; }
        if(A=="next" && TestPhase==4) { Enter(Step==E::Test?E::Heat:E::Fold); return true; }
    } else if(Step==E::Fold && A=="fold" && FoldCount<3) { Animate("fold",.7f); return true;
    } else if(Step==E::Clamp) {
        if(A=="cloth" && ClampPhase==0) { Animate("clamp",.6f); return true; }
        if(A=="lid" && ClampPhase==1) { Animate("clamp",.65f); return true; }
        if(ClampPhase==2 && ((A=="clipL"&&!ClipLeft)||(A=="clipR"&&!ClipRight))) {
            (A=="clipL"?ClipLeft:ClipRight)=true;
            if(ClipLeft&&ClipRight) Animate("clips",.65f); return true;
        }
    } else if(Step==E::Dye) {
        if(A=="dye" && Ready && ClipLeft && ClipRight && DyeDepth<4) { Animate("dye",7.8f); return true; }
        if(A=="finish" && DyeDepth>0) { Enter(E::Wash); return true; }
    } else if(Step==E::Wash) {
        if(A=="wash" && WashPhase<2) { Animate("wash",WashPhase==0?1.6f:1.2f); return true; }
        if(WashPhase==2) {
            if(A=="openL"&&!OpenLeft) { OpenLeft=true; return true; }
            if(A=="openR"&&!OpenRight) { OpenRight=true; return true; }
            if(A=="next" && OpenLeft&&OpenRight) { Animate("reveal",3.f); return true; }
        }
    } else if(Step==E::Reveal && A=="next") { Enter(E::Letter); return true;
    } else if(Step==E::Letter && A=="next") { Enter(E::Pack); return true;
    } else if(Step==E::Pack && A=="next") { Animate("pack",3.f); return true;
    } else if(Step==E::End) {
        if(A=="review") { Enter(E::Review); return true; }
        if(A=="restart") { Reset(); return true; }
    } else if(Step==E::Review && A=="next") { Enter(E::End); return true; }
    if(A=="heat" && (Step==E::Heat || Step==E::Dye) && !Ready && Fuel>0 && DyeDepth<4) { --Fuel; Animate("heat",2.1f); return true; }
    return false;
}
bool FIndigoSession::Advance(float Delta) {
    if(!Busy()) return false;
    Remaining=FMath::Max(0.f,Remaining-FMath::Max(0.f,Delta));
    if(Busy()) return false;
    FString Event=Pending; Pending.Empty(); Duration=0;
    using E=EIndigoStep;
    if(Event=="test") TestPhase=4;
    else if(Event=="heat") { Ready=true; if(Step==E::Heat) { TestPhase=0; Enter(E::Retest); } }
    else if(Event=="fold") { ++FoldCount; if(FoldCount==3) Enter(E::Clamp); }
    else if(Event=="clamp") ++ClampPhase;
    else if(Event=="clips") Enter(E::Dye);
    else if(Event=="dye") { DyeDepth=FMath::Min(4,DyeDepth+1); Ready=false; }
    else if(Event=="wash") ++WashPhase;
    else if(Event=="reveal") Enter(E::Reveal);
    else if(Event=="pack") { Enter(E::FilmOne); Animate("film1",15.f); }
    else if(Event=="film1") { Enter(E::FilmTwo); Animate("film2",15.f); }
    else if(Event=="film2") { Enter(E::Sky); Animate("sky",4.5f); }
    else if(Event=="sky") Enter(E::End);
    return true;
}
FString FIndigoSession::Title() const {
    static const TCHAR* Titles[]={TEXT("一件留着旧样的委托"),TEXT("把花纹放在两侧"),TEXT("先用一条试布"),TEXT("给染缸补一点温"),TEXT("再试一条，看看蓝"),TEXT("沿折线，慢慢叠齐"),TEXT("布与版，合在一起"),TEXT("让蓝，一层层留下"),TEXT("清洗，然后展开"),TEXT("这就是你的蓝"),TEXT("旧样，原来是这样"),TEXT("把心意寄出去"),TEXT("仍在使用的蓝"),TEXT("留在记忆里的蓝"),TEXT("蓝色，铺展为天空"),TEXT("一床被面，一份心意"),TEXT("再看一眼，你的作品")};
    return Titles[static_cast<int32>(Step)];
}
FString FIndigoSession::Hint() const {
    if(Busy()) {
        if(Pending=="heat") return TEXT("谷壳慢慢燃起来，等染缸暖一暖。 ");
        if(Pending=="dye") return Progress()<.44f?TEXT("慢慢浸入染缸，让染液进入布里。"):Progress()<.62f?TEXT("提出，等水滴落。"):TEXT("见了空气，暗绿慢慢显出蓝。");
        if(Pending=="test") return TEXT("提出试布，看看它在空气中怎样显色。");
        if(Pending=="reveal") return TEXT("打开花版，先露出一角，再慢慢铺开。");
        if(Pending=="pack") return TEXT("把这床被面仔细叠好。愿这份心意平安送到。");
        if(Pending=="fold") return TEXT("沿着折线，把布边慢慢叠齐。");
    }
    using E=EIndigoStep;
    if(Step==E::Receive) return SawLetter?TEXT("“旧样留一处，其余花纹请你挑。做好后寄回家里。”"):TEXT("这块旧样要留下。先看看她在单子上写了什么。");
    if(Step==E::Pattern) return TEXT("拖 A / B / C 到左右区域，或先点花版，再点区域。中间旧样固定。");
    if(Step==E::Test) return TestPhase==4?TEXT("颜色偏淡、不均匀，是缸温不够。添一份谷壳再试。"):TEXT("舀液、浸入、提出。先试一小条，看看能不能显出蓝。");
    if(Step==E::Heat) return TEXT("缸温不足，添一份谷壳，暖好了再试。");
    if(Step==E::Retest) return TestPhase==4?TEXT("这次的蓝清楚了。第一次补温也支持第一段正式染色。"):TEXT("缸温合适了，再试一条，看看这次的颜色。");
    if(Step==E::Fold) return TEXT("把当前布边拖到折线。布折齐了，才好放进花版。");
    if(Step==E::Clamp) return TEXT("先放布，再合版。左右夹具都扣紧，才能入染。");
    if(Step==E::Dye) return DyeDepth==4?TEXT("已经是第四档了。点击收工，把蓝留在这里。"):Ready?TEXT("缸温合适，可以入染。颜色到了想要的程度，就可以收手。"):TEXT("缸温降下来了。继续染要先添谷壳；也可以收工。");
    if(Step==E::Wash) return TEXT("清洗、沥水，再松开两侧夹具，把花版轻轻打开。");
    if(Step==E::Reveal || Step==E::Review) return TEXT("两边是我选的花纹，中间还是她熟悉的旧样。");
    if(Step==E::Letter) return TEXT("原来，这处旧样，是母亲留给女儿的一份心意。");
    if(Step==E::Pack) return TEXT("希望这床被面，能陪她过好往后的日子。");
    if(Step==E::FilmOne || Step==E::FilmTwo) return TEXT("地域片分镜占位：正式地区、工艺和影像待核验后接入。");
    return TEXT("旧样留了一处，新的日子，也有了自己的花纹。");
}
