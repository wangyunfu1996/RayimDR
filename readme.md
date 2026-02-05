代码仓库
git@github.com:wangyunfu1996/RayimDR.git

UI组件 ElaWidgetTools
git@github.com:wangyunfu1996/ElaWidgetTools.git

探测器 IRay NDT1717MA
SDK版本 903-340-32_NDT_SDK_ReleasePackage_4.4.4.10041.zip


# 校正流程
### Create Offset
Cmd_OffsetGeneration

### Create Gain
Cmd_GainInit
Cmd_StartAcq
开启射线源，设置到 70kV，期望 12000
等待 nCenterValue 到达期望值
Cmd_GainSelectAll 0 5(Attr_GainTotalFrames)
Cmd_GainGeneration
Cmd_FinishGenerationProcess

### Create Defect
#define Attr_CurrentTransaction	5005

#define Cmd_DefectInit	3007
#define Attr_DefectTotalFrames	5032
#define Attr_DefectValidFrames	5033

40kV 期望值 1200
#define Cmd_StartAcq	1004
开启射线 等待 nCenterValue 达到期望值
#define Cmd_DefectSelectAll	3033 (0, 1)
关闭射线
#define Cmd_ForceDarkContinuousAcq	1010 (1)

60kV 期望值 2000
#define Cmd_StartAcq	1004
开启射线 等待 nCenterValue 达到期望值
#define Cmd_DefectSelectAll	3033 (1, 1)
关闭射线
#define Cmd_ForceDarkContinuousAcq	1010 (1)

70kV 期望值 4000
#define Cmd_StartAcq	1004
开启射线 等待 nCenterValue 达到期望值
#define Cmd_DefectSelectAll	3033 (2, 1)
关闭射线
#define Cmd_ForceDarkContinuousAcq	1010 (1)

80kV 期望值 8000
#define Cmd_StartAcq	1004
开启射线 等待 nCenterValue 达到期望值
#define Cmd_DefectSelectAll	3033 (3, 5)
关闭射线
#define Cmd_ForceDarkContinuousAcq	1010 (5)

#define Cmd_DefectGeneration	3010
#define Cmd_FinishGenerationProcess	3015


Mode6 
Gain 70kV 0.5mA
Defect 
stage1 40kv 0.2mA
stage2 50kv 0.2mA
stage2 65kv 0.2mA
stage2 80kv 0.25mA


### 射线源
有线 192.168.10.1
本机 (手动)
192.168.10.101
255.255.255.0
无线 192.168.1.1
本机 自动

### 探测器 
有线
192.168.10.2:27888（手动）
255.255.255.0
本机
192.168.10.101


射线源电量的判断
20-21 一格电 不允许开光
21-22 二格电 不允许开光
22-23 三格电 不允许开光
23 四格电 不允许开光
