# 维护铁律：compat 永远 = main + 一个"删 wsclient、换 matrix 桩"的提交
# compat 上写的新提交会被自动搬到 main 上，然后 compat 按 main 重建
$ErrorActionPreference = 'Stop'

$FILES = @('src/wsclient.cpp')
$MARK = 'chore: clean'

function Invoke-Git {
    & git @args
    if ($LASTEXITCODE -ne 0) { throw ('git ' + ($args -join ' ') + ' 失败') }
}

$root = (& git rev-parse --show-toplevel)
if ($LASTEXITCODE -ne 0) { Write-Host '这里不是 git 仓库'; exit 1 }
Set-Location $root

& git diff-index --quiet HEAD --
if ($LASTEXITCODE -ne 0) { Write-Host '有未提交的改动，先提交或 stash'; exit 1 }

$START = (& git rev-parse --abbrev-ref HEAD)

try {
    Invoke-Git fetch origin --prune
    Invoke-Git switch -q main
    Invoke-Git merge --ff-only origin/main

    # compat 上除了那个删文件的提交，其余都是真活儿，按顺序挑到 main
    foreach ($c in @(& git rev-list --reverse main..compat)) {
        if ((& git log -1 --format=%s $c) -eq $MARK) { continue }
        Write-Host ('>> 把 ' + (& git log -1 --format='%h %s' $c) + ' 搬到 main')
        Invoke-Git cherry-pick --empty=drop $c
    }

    # 已经正好是 main + 那个提交，就什么都不用做
    $synced = ((& git rev-list --count main..compat) -eq '1') -and
        ((& git log -1 --format=%s compat) -eq $MARK) -and
        ((& git rev-parse compat^) -eq (& git rev-parse main))
    if ($synced) {
        Write-Host 'compat 已经是最新'
    } else {
        Invoke-Git branch -f compat main
        Invoke-Git switch -q compat
        Invoke-Git rm -q @FILES
        $stub = @'
#include "hal/matrix.h"

namespace matrix
{
    void init(void)
    {
        begin();
    }

    void update(void)
    {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate < 200)
            return;
        lastUpdate = now;

        scanDisplay();
    }
}
'@
        $stub = $stub -replace "`r`n", "`n"
        $utf8 = New-Object System.Text.UTF8Encoding $false
        [IO.File]::WriteAllText((Join-Path $PWD 'src/hal/matrix.cpp'), ($stub + "`n"), $utf8)
        Invoke-Git add src/hal/matrix.cpp
        Invoke-Git commit -q -m $MARK
    }

    Invoke-Git switch -q $START

    if (-not $env:SKIP_PUSH) {
        Invoke-Git push --force-with-lease origin main compat
    }

    Invoke-Git log --oneline --graph -3 compat
} catch {
    & git cherry-pick --abort 2>&1 | Out-Null
    & git switch -q $START 2>&1 | Out-Null
    Write-Host $_.Exception.Message
    exit 1
}
