# csgo-internal

Base HydraWare para Windows Win32 x86, com as implementações de **bhop**, **jumpbug** e **edgebug** portadas do Hotwheels. O restante do pipeline de CreateMove, prediction, legitbot, edgejump, configurações e hooks permanece HydraWare. Os toggles de fast duck preservados são `m_no_crouch_cooldown` e `m_auto_duck`.

A compilação é executada pelo workflow `Build HydraWare Win32 DLL` em `.github/workflows/build-windows.yml` usando MSBuild, Visual Studio 2022 e D3DX9 x86.
