# Emotiv Interface

Emotiv Interface is a C++ application that serves a REST/JSON API exposing mental commands read from Emotiv mobile EEG headset.

This repository is part of the **Hot and Cold Game**. 

The Hot and Cold is a game where the player commands a wheeled robot (Turtlebot3 Waffle) with the brain (EMOTIV Insight 5 channel mobile EEG) guiding it to a goal. 

### Hot and Cold Game Repositories:

- emotivinterface
- emotivwebserver
- jogo_quente_frio

### Requirements:

Emotiv Epoc or Insight EEG headset (www.emotiv.com)  
Visual Studio 2017 (v141)  
Emotiv Community SDK (https://github.com/Emotiv/community-sdk)  
JsonCpp (embeded in sources)  
WebServer (embeded in sources)  
Emotiv Xavier Control Panel, only for training command actions (https://drive.google.com/open?id=147dwK88pOQe6nuLj-MugHCnKNpJCtwDN)  

### Configuration

Before building please define your Emotiv Cloud credentials, profile and .emu file path editing EmotivInterface/Emotiv/Emotiv.cpp. The .emu file is generated on your local system by Emotiv Xavier Control panel after training command actions.

### Building:

Please, set all references (Emotiv Community SDK include folder, JsonCpp root folder and WebServer root folder) paths at Visual Studio project properties: "Propriedades de Configuração / C++ / Geral / Diretórios de Inclusão Adicionais"

Also set the path to Emotiv Community SDK lib folder at "Propriedades de Configuração / Vinculador / Geral / Diretórios de Biblioteca Adicionais" 

### Building Troubleshooting

Some compilation errors can ben solved adding CRT_SECURE_NO_WARNINGS flag to "Propriedades de Configuração / C++ / Pré-processador / Definições do Pré-processador"

Also disabling pre-compiled headers at "Propriedades de Configuração / C++ / Cabeçalhos Pré-compilados / Cabeçalho Pré-compilado / Não Usar Cabeçalhos Pré-compilados".

Maybe it will be also required to change Windows SDK version at "Propriedades de Configuração / Geral / Versão do SDK do Windows". Tested with Windows SDK v10.0.17763.0


