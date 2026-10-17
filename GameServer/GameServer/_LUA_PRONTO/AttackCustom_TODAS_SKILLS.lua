--[[
    Mu NaVeia - /attack com escolha de skill por classe
    Baseado no Skill.txt do servidor (DW, DK, FE, MG, DL, SU).

    REQUER NO LuaUser.cpp:
        getAttackCustomSkill()
        setAttackCustomSkill()

    COMANDOS:
        /attack                 -> liga/desliga usando a skill padrao da classe
        /attack on              -> liga com a skill padrao
        /attack off             -> desliga
        /attack list            -> mostra as skills de ataque disponiveis da classe
        /attack <ID>            -> seleciona pelo ID e liga
        /attack <nome>          -> seleciona pelo nome/apelido e liga

    EXEMPLOS:
        /attack evil
        /attack inferno
        /attack 14
        /attack twist
        /attack triple
        /attack chain
]]--

AttackCustomLua = {}

ATTACK_MESSAGE_TYPE = 1

-- =====================================================================================
-- SKILLS DE ATAQUE POR CLASSE
-- Usei somente skills ofensivas/adequadas para auto-attack.
-- Buffs, teleportes, summons e skills administrativas/eventuais ficaram de fora.
-- =====================================================================================

ATTACK_CLASS_NAME = {
    [0] = "DW/SM",
    [1] = "DK/BK",
    [2] = "Elf/ME",
    [3] = "MG/DM",
    [4] = "DL/LE",
    [5] = "Summoner/BS"
}

ATTACK_SKILLS = {

    [0] = {
        Default = 9,
        List = {
            {Id=1,   Name="Poison",              Alias={"poison"}},
            {Id=2,   Name="Meteorite",           Alias={"meteorite","meteor"}},
            {Id=3,   Name="Lightning",           Alias={"lightning"}},
            {Id=4,   Name="Fire Ball",           Alias={"fireball","fire ball"}},
            {Id=5,   Name="Flame",               Alias={"flame"}},
            {Id=7,   Name="Ice",                 Alias={"ice"}},
            {Id=8,   Name="Twister",             Alias={"twister"}},
            {Id=9,   Name="Evil Spirit",         Alias={"evil","evil spirit","evilspirit"}},
            {Id=10,  Name="Hell Fire",           Alias={"hellfire","hell fire"}},
            {Id=11,  Name="Power Wave",          Alias={"powerwave","power wave"}},
            {Id=12,  Name="Aqua Beam",           Alias={"aquabeam","aqua beam"}},
            {Id=13,  Name="Blast",               Alias={"blast"}},
            {Id=14,  Name="Inferno",             Alias={"inferno"}},
            {Id=17,  Name="Energy Ball",         Alias={"energyball","energy ball"}},
            {Id=38,  Name="Decay",               Alias={"decay"}},
            {Id=39,  Name="Ice Storm",           Alias={"icestorm","ice storm"}}
        }
    },

    [1] = {
        Default = 41,
        List = {
            {Id=19,  Name="Falling Slash",       Alias={"falling","falling slash"}},
            {Id=20,  Name="Lunge",               Alias={"lunge"}},
            {Id=21,  Name="Uppercut",             Alias={"uppercut"}},
            {Id=22,  Name="Cyclone",              Alias={"cyclone"}},
            {Id=23,  Name="Slash",                Alias={"slash"}},
            {Id=41,  Name="Twisting Slash",       Alias={"twist","twisting","twisting slash"}},
            {Id=42,  Name="Rageful Blow",         Alias={"rageful","rageful blow"}},
            {Id=43,  Name="Death Stab",           Alias={"deathstab","death stab"}},
            {Id=44,  Name="Crescent Moon Slash",  Alias={"crescent","crescent moon slash"}},
            {Id=47,  Name="Impale",               Alias={"impale"}},
            {Id=49,  Name="Fire Breath",          Alias={"firebreath","fire breath"}},
            {Id=232, Name="Frozen Stab",          Alias={"frozen","frozen stab"}}
        }
    },

    [2] = {
        Default = 235,
        List = {
            {Id=24,  Name="Triple Shot",          Alias={"triple","triple shot","tripleshot"}},
            {Id=46,  Name="Starfall",             Alias={"starfall"}},
            {Id=51,  Name="Ice Arrow",            Alias={"icearrow","ice arrow"}},
            {Id=52,  Name="Penetration",          Alias={"penetration"}},
            {Id=235, Name="Five Shot",            Alias={"five","five shot","fiveshot","multishot","multi shot"}}
        }
    },

    [3] = {
        Default = 9,
        List = {
            {Id=1,   Name="Poison",               Alias={"poison"}},
            {Id=2,   Name="Meteorite",            Alias={"meteorite","meteor"}},
            {Id=3,   Name="Lightning",            Alias={"lightning"}},
            {Id=4,   Name="Fire Ball",            Alias={"fireball","fire ball"}},
            {Id=5,   Name="Flame",                Alias={"flame"}},
            {Id=7,   Name="Ice",                  Alias={"ice"}},
            {Id=8,   Name="Twister",              Alias={"twister"}},
            {Id=9,   Name="Evil Spirit",          Alias={"evil","evil spirit","evilspirit"}},
            {Id=10,  Name="Hell Fire",            Alias={"hellfire","hell fire"}},
            {Id=11,  Name="Power Wave",           Alias={"powerwave","power wave"}},
            {Id=12,  Name="Aqua Beam",            Alias={"aquabeam","aqua beam"}},
            {Id=13,  Name="Blast",                Alias={"blast"}},
            {Id=14,  Name="Inferno",              Alias={"inferno"}},
            {Id=17,  Name="Energy Ball",          Alias={"energyball","energy ball"}},
            {Id=19,  Name="Falling Slash",        Alias={"falling","falling slash"}},
            {Id=20,  Name="Lunge",                Alias={"lunge"}},
            {Id=21,  Name="Uppercut",             Alias={"uppercut"}},
            {Id=22,  Name="Cyclone",              Alias={"cyclone"}},
            {Id=23,  Name="Slash",                Alias={"slash"}},
            {Id=39,  Name="Ice Storm",            Alias={"icestorm","ice storm"}},
            {Id=41,  Name="Twisting Slash",       Alias={"twist","twisting","twisting slash"}},
            {Id=47,  Name="Impale",               Alias={"impale"}},
            {Id=55,  Name="Fire Slash",           Alias={"fireslash","fire slash"}},
            {Id=56,  Name="Power Slash",          Alias={"powerslash","power slash"}},
            {Id=57,  Name="Spiral Slash",         Alias={"spiral","spiral slash"}},
            {Id=73,  Name="Mana Rays",            Alias={"manarays","mana rays"}},
            {Id=236, Name="Sword Slash",          Alias={"swordslash","sword slash"}},
            {Id=237, Name="Lightning Storm",      Alias={"lightningstorm","lightning storm"}}
        }
    },

    [4] = {
        Default = 61,
        List = {
            {Id=60,  Name="Force",                Alias={"force"}},
            {Id=61,  Name="Fire Burst",           Alias={"fireburst","fire burst"}},
            {Id=65,  Name="Electric Spark",       Alias={"electricspark","electric spark"}},
            {Id=66,  Name="Force Wave",           Alias={"forcewave","force wave"}},
            {Id=74,  Name="Fire Blast",           Alias={"fireblast","fire blast"}},
            {Id=78,  Name="Fire Scream",          Alias={"firescream","fire scream"}},
            {Id=238, Name="Birds",                Alias={"birds"}}
        }
    },

    [5] = {
        Default = 215,
        List = {
            {Id=2,   Name="Meteorite",            Alias={"meteorite","meteor"}},
            {Id=4,   Name="Fire Ball",            Alias={"fireball","fire ball"}},
            {Id=7,   Name="Ice",                  Alias={"ice"}},
            {Id=9,   Name="Evil Spirit",          Alias={"evil","evil spirit","evilspirit"}},
            {Id=11,  Name="Power Wave",           Alias={"powerwave","power wave"}},
            {Id=19,  Name="Falling Slash",        Alias={"falling","falling slash"}},
            {Id=20,  Name="Lunge",                Alias={"lunge"}},
            {Id=21,  Name="Uppercut",             Alias={"uppercut"}},
            {Id=22,  Name="Cyclone",              Alias={"cyclone"}},
            {Id=23,  Name="Slash",                Alias={"slash"}},
            {Id=45,  Name="Mana Glaive",          Alias={"managlaive","mana glaive"}},
            {Id=214, Name="Drain Life",            Alias={"drain","drainlife","drain life"}},
            {Id=215, Name="Chain Lightning",       Alias={"chain","chainlightning","chain lightning"}},
            {Id=216, Name="Electric Surge",       Alias={"surge","electricsurge","electric surge"}},
            {Id=223, Name="Sahamutt",             Alias={"sahamutt"}},
            {Id=224, Name="Neil",                 Alias={"neil"}},
            {Id=225, Name="Ghost Phantom",        Alias={"ghost","phantom","ghost phantom"}},
            {Id=230, Name="Red Storm",            Alias={"redstorm","red storm"}}
        }
    }
}

local function AttackSend(aIndex, text)
    SendMessage(text, aIndex, ATTACK_MESSAGE_TYPE)
end

local function Trim(text)
    if text == nil then
        return ""
    end

    text = tostring(text)
    text = string.gsub(text, "^%s+", "")
    text = string.gsub(text, "%s+$", "")
    return text
end

local function Normalize(text)
    if text == nil then
        return ""
    end

    -- Algumas builds passam Arguments como tabela em vez de string.
    if type(text) == "table" then
        local parts = {}

        for _, value in pairs(text) do
            table.insert(parts, tostring(value))
        end

        text = table.concat(parts, " ")
    else
        text = tostring(text)
    end

    text = string.lower(text)

    -- Limpeza geral.
    text = string.gsub(text, "^%s+", "")
    text = string.gsub(text, "%s+$", "")
    text = string.gsub(text, "%s+", " ")

    -- Algumas implementacoes de Commands.Register passam o texto inteiro:
    -- "/attack list" em vez de somente "list".
    text = string.gsub(text, "^/attack%s*", "")
    text = string.gsub(text, "^attack%s*", "")

    -- Outras podem mandar separadores.
    text = string.gsub(text, "^[=:,;%-]+%s*", "")
    text = string.gsub(text, "^%s+", "")
    text = string.gsub(text, "%s+$", "")

    return text
end

local function RawArgumentsToString(value)
    if value == nil then
        return "nil"
    end

    if type(value) == "table" then
        local parts = {}

        for key, val in pairs(value) do
            table.insert(parts, tostring(key) .. "=" .. tostring(val))
        end

        return "table{" .. table.concat(parts, ",") .. "}"
    end

    return tostring(value)
end

local function GetBaseClass(rawClass)
    if rawClass >= 80 then
        return 5 -- reservado para Summoner caso essa build use classe 80+
    end

    if rawClass >= 64 then
        return 4 -- DL
    end

    if rawClass >= 48 then
        return 3 -- MG
    end

    if rawClass >= 32 then
        return 2 -- ELF
    end

    if rawClass >= 16 then
        return 1 -- DK
    end

    return 0 -- DW
end

local function GetClassConfig(player)
    local rawClass = player:getClass()
    local baseClass = GetBaseClass(rawClass)

    return ATTACK_SKILLS[baseClass], baseClass, rawClass
end

local function FindSkillById(config, skillId)
    for _, skill in ipairs(config.List) do
        if skill.Id == skillId then
            return skill
        end
    end

    return nil
end

local function FindSkillByName(config, name)
    name = Normalize(name)

    for _, skill in ipairs(config.List) do
        if Normalize(skill.Name) == name then
            return skill
        end

        if skill.Alias ~= nil then
            for _, alias in ipairs(skill.Alias) do
                if Normalize(alias) == name then
                    return skill
                end
            end
        end
    end

    return nil
end

function AttackCustomLua.Disable(aIndex, player)
    if player.stopCustomAttack ~= nil then
        player:stopCustomAttack()
    else
        -- fallback para compatibilidade
        player:setInOffAttack(0)
    end

    AttackSend(aIndex, "[Attack] Auto ataque DESATIVADO.")
end

function AttackCustomLua.Enable(aIndex, player, skill)
    if skill == nil then
        AttackSend(aIndex, "[Attack] Skill invalida para sua classe.")
        return
    end

    if player.startCustomAttackSkill == nil then
        AttackSend(aIndex, "[Attack] ERRO: GameServer sem startCustomAttackSkill().")
        AttackSend(aIndex, "[Attack] Compile a source corrigida antes de usar esta script.")
        return
    end

    if player.hasCustomAttackSkill ~= nil then
        local hasSkill = player:hasCustomAttackSkill(skill.Id)

        if hasSkill == 0 then
            AttackSend(aIndex, string.format(
                "[Attack] Voce nao possui a skill %s [ID %d].",
                skill.Name,
                skill.Id
            ))
            return
        end
    end

    local result = player:startCustomAttackSkill(skill.Id)

    if result == 0 then
        AttackSend(aIndex, string.format(
            "[Attack] Nao foi possivel ativar %s [ID %d].",
            skill.Name,
            skill.Id
        ))
        return
    end

    AttackSend(aIndex, string.format(
        "[Attack] ATIVADO: %s [ID %d]",
        skill.Name,
        skill.Id
    ))
end

function AttackCustomLua.ShowList(aIndex, player)
    local config, class, rawClass = GetClassConfig(player)

    if config == nil then
        AttackSend(aIndex, string.format(
            "[Attack] Classe base %d (raw %d) sem configuracao.",
            class,
            rawClass
        ))
        return
    end

    AttackSend(aIndex, string.format(
        "[Attack] Skills disponiveis para %s:",
        ATTACK_CLASS_NAME[class] or tostring(class)
    ))

    -- Envia em blocos pequenos para nao estourar o tamanho do chat.
    local line = ""

    for i, skill in ipairs(config.List) do
        local item = string.format("%d=%s", skill.Id, skill.Name)

        if line == "" then
            line = item
        elseif string.len(line) + string.len(item) + 3 > 85 then
            AttackSend(aIndex, line)
            line = item
        else
            line = line .. " | " .. item
        end
    end

    if line ~= "" then
        AttackSend(aIndex, line)
    end

    AttackSend(aIndex, "Use: /attack ID ou /attack nome")
end

function AttackCustomLua.Command(aIndex, Arguments)
    local player = User.new(aIndex)

    if player == nil or player:getConnected() < 2 then
        return
    end

    local config, class, rawClass = GetClassConfig(player)

    if config == nil then
        AttackSend(aIndex, string.format(
            "[Attack] Classe base %d (raw %d) sem configuracao.",
            class,
            rawClass
        ))
        return
    end

    local args = Normalize(Arguments)

    -- Diagnostico da sua build.
    if args == "debug" then
        AttackSend(aIndex, string.format(
            "[Attack DEBUG] rawClass=%d baseClass=%d",
            rawClass,
            class
        ))
        AttackSend(aIndex, "[Attack DEBUG] type=" .. type(Arguments))
        AttackSend(aIndex, "[Attack DEBUG] raw=" .. RawArgumentsToString(Arguments))
        AttackSend(aIndex, "[Attack DEBUG] parsed=" .. args)
        return
    end

    -- /attack = toggle usando a skill atual, ou a padrao se nenhuma valida estiver selecionada
    if args == "" or args == "null" then
        if player:getInOffAttack() ~= 0 then
            AttackCustomLua.Disable(aIndex, player)
            return
        end

        local currentId = player:getAttackCustomSkill()
        local currentSkill = FindSkillById(config, currentId)

        if currentSkill == nil then
            currentSkill = FindSkillById(config, config.Default)
        end

        AttackCustomLua.Enable(aIndex, player, currentSkill)
        return
    end

    if args == "off" or args == "0" then
        AttackCustomLua.Disable(aIndex, player)
        return
    end

    if args == "on" or args == "default" or args == "padrao" then
        local defaultSkill = FindSkillById(config, config.Default)
        AttackCustomLua.Enable(aIndex, player, defaultSkill)
        return
    end

    if args == "list" or args == "lista" or args == "skills" then
        AttackCustomLua.ShowList(aIndex, player)
        return
    end

    -- Primeiro tenta por ID
    local skillId = tonumber(args)

    if skillId ~= nil then
        skillId = math.floor(skillId)

        local skill = FindSkillById(config, skillId)

        if skill == nil then
            AttackSend(aIndex, string.format(
                "[Attack] Skill ID %d nao permitida para %s.",
                skillId,
                ATTACK_CLASS_NAME[class] or tostring(class)
            ))
            AttackSend(aIndex, "Digite /attack list para ver as skills.")
            return
        end

        AttackCustomLua.Enable(aIndex, player, skill)
        return
    end

    -- Depois tenta pelo nome/apelido
    local skill = FindSkillByName(config, args)

    if skill == nil then
        AttackSend(aIndex, "[Attack] Skill nao encontrada para sua classe.")
        AttackSend(aIndex, "[Attack DEBUG] recebido: " .. args)
        AttackSend(aIndex, "Digite /attack list para ver as skills.")
        return
    end

    AttackCustomLua.Enable(aIndex, player, skill)
end

Commands.Register("/attack", AttackCustomLua.Command)

LogAdd("[AttackCustomLua] /attack multi-skill carregado.")

return AttackCustomLua
