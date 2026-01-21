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
Cmd_DefectInit

40kV 期望值 1200
Cmd_StartAcq
开启射线 等待 nCenterValue 达到期望值
Cmd_DefectSelectAll 0 5(Attr_GainTotalFrames)
关闭射线
Cmd_ForceDarkContinuousAcq

60kV 期望值 2000
Cmd_StartAcq
开启射线 等待 nCenterValue 达到期望值
Cmd_DefectSelectAll 0 5(Attr_GainTotalFrames)
关闭射线
Cmd_ForceDarkContinuousAcq

70kV 期望值 4000
Cmd_StartAcq
开启射线 等待 nCenterValue 达到期望值
Cmd_DefectSelectAll 0 5(Attr_GainTotalFrames)
关闭射线
Cmd_ForceDarkContinuousAcq

80kV 期望值 8000
Cmd_StartAcq
开启射线 等待 nCenterValue 达到期望值
Cmd_DefectSelectAll 0 5(Attr_GainTotalFrames)
关闭射线
Cmd_ForceDarkContinuousAcq

Cmd_DefectGeneration
Cmd_FinishGenerationProcess