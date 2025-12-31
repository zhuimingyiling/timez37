static char *getplayer(const char *类名)
{
    if (strstr(类名, "detective") != NULL)
    {
        return (char *)"[侦探]";
    }
    else if (strstr(类名, "w_qx") != NULL)
    {
        return (char *)"[气象学家]";
    }
    else if (strstr(类名, "w_gjs") != NULL)
    {
        return (char *)"[弓箭手]";
    }
    else if (strstr(类名, "npc_deluosi_dress_ghost") != NULL)
    {
        return (char *)"[联觉记者]";
    }
    else if (strstr(类名, "h55_survivor_w_xiangshuishi") != NULL)
    {
        return (char *)"[调香师]";
    }
    else if (strstr(类名, "h55_survivor_w_bdz") != NULL)
    {
        return (char *)"[空军]";
    }
    else if (strstr(类名, "h55_survivor_m_yxz_bo") != NULL)
    {
        return (char *)"[佣兵]";
    }
    else if (strstr(类名, "dm65_survivor_m_bo") != NULL)
    {
        return (char *)"[幸运儿]";
    }
    else if (strstr(类名, "h55_survivor_m_qiutu") != NULL)
    {
        return (char *)"[囚徒]";
    }
    else if (strstr(类名, "dm65_survivor_w_yiyaoshi") != NULL)
    {
        return (char *)"[医生]";
    }
    else if (strstr(类名, "h55_survivor_m_xcz") != NULL)
    {
        return (char *)"[冒险家]";
    }
    else if (strstr(类名, "h55_survivor_w_cp") != NULL)
    {
        return (char *)"[心理学家]";
    }
    else if (strstr(类名, "h55_survivor_m_cp") != NULL)
    {
        return (char *)"[病患]";
    }
    else if (strstr(类名, "h55_survivor_w_hlz") != NULL)
    {
        return (char *)"[园丁]";
    }
    else if (strstr(类名, "h55_survivor_m_zbs") != NULL)
    {
        return (char *)"[先知]";
    }
    else if (strstr(类名, "h55_survivor_w_fxt") != NULL || strstr(类名, "h55_survivor_tqz") != NULL)
    {
        return (char *)"[机械师]";
    }
    else if (strstr(类名, "h55_survivor_puppet") != NULL)
    {
        return (char *)"[机器人]";
    }
    else if (strstr(类名, "h55_survivor_m_shoumu") != NULL)
    {
        return (char *)"[守墓人]";
    }
    else if (strstr(类名, "h55_survivor_m_ydy") != NULL)
    {
        return (char *)"[前锋]";
    }
    else if (strstr(类名, "h55_survivor_w_wht") != NULL)
    {
        return (char *)"[舞女]";
    }
    else if (strstr(类名, "h55_survivor_m_niuzai") != NULL)
    {
        return (char *)"[牛仔]";
    }
    else if (strstr(类名, "h55_survivor_m_rls") != NULL)
    {
        return (char *)"[入殓师]";
    }
    else if (strstr(类名, "h55_survivor_w_jyz") != NULL)
    {
        return (char *)"[盲女]";
    }
    else if (strstr(类名, "h55_survivor_m_yc") != NULL)
    {
        return (char *)"[邮差]";
    }
    else if (strstr(类名, "h55_survivor_w_jisi") != NULL)
    {
        return (char *)"[祭司]";
    }
    else if (strstr(类名, "h55_survivor_m_ldz") != NULL)
    {
        return (char *)"[魔术师]";
    }
    else if (strstr(类名, "survivor_girl") != NULL)
    {
        return (char *)"[小女孩]";
    }
    else if (strstr(类名, "h55_survivor_w_zhoushu") != NULL)
    {
        return (char *)"[咒术师]";
    }
    else if (strstr(类名, "h55_survivor_m_kantan") != NULL)
    {
        return (char *)"[勘探员]";
    }
    else if (strstr(类名, "h55_survivor_m_yeren") != NULL)
    {
        return (char *)"[野人]";
    }
    else if (strstr(类名, "h55_survivor_m_zaji") != NULL)
    {
        return (char *)"[杂技演员]";
    }
    else if (strstr(类名, "h55_survivor_m_dafu") != NULL)
    {
        return (char *)"[大副]";
    }
    else if (strstr(类名, "h55_survivor_w_tiaojiu") != NULL)
    {
        return (char *)"[调酒师]";
    }
    else if (strstr(类名, "h55_survivor_w_kunchong") != NULL)
    {
        return (char *)"[昆虫学家]";
    }
    else if (strstr(类名, "h55_survivor_m_artist") != NULL)
    {
        return (char *)"[画家]";
    }
    else if (strstr(类名, "h55_survivor_m_jiqiu") != NULL)
    {
        return (char *)"[击球手]";
    }
    else if (strstr(类名, "h55_survivor_w_shangren") != NULL)
    {
        return (char *)"[玩具商]";
    }
    else if (strstr(类名, "h55_survivor_m_bzt") != NULL)
    {
        return (char *)"[小说家]";
    }
    else if (strstr(类名, "h55_survivor_m_yinyue") != NULL)
    {
        return (char *)"[作曲家]";
    }
    else if (strstr(类名, "h55_survivor_m_spjk") != NULL || strstr(类名, "h55_pendant_huojian") != NULL)
    {
        return (char *)"[哭泣小丑]";
    }
    else if (strstr(类名, "h55_survivor_w_gd") != NULL)
    {
        return (char *)"[古董商]";
    }
    else if (strstr(类名, "h55_survivor_m_niexi") != NULL)
    {
        return (char *)"[教授]";
    }
    else if (strstr(类名, "h55_survivor_m_fxj") != NULL)
    {
        return (char *)"[飞行家]";
    }
    else if (strstr(类名, "h55_survivor_w_deluosi") != NULL)
    {
        return (char *)"[记者]";
    }
    else if (strstr(类名, "h55_survivor_m_muou") != NULL)
    {
        return (char *)"[木偶师]";
    }
    else if (strstr(类名, "h55_survivor_m_it") != NULL)
    {
        return (char *)"[律师]";
    }
    else if (strstr(类名, "h55_survivor_m_qd") != NULL)
    {
        return (char *)"[慈善家]";
    }
    else if (strstr(类名, "h55_survivor_w_ll") != NULL)
    {
        return (char *)"[拉拉队员]";
    }
    else if (strstr(类名, "survivor_w_fl") != NULL)
    {
        return (char *)"[法罗女士]";
    }
    else if (strstr(类名, "h55_survivor_m_xf") != NULL)
    {
        return (char *)"[火灾调查员]";
    }
    else if (strstr(类名, "h55_survivor_m_dxzh") != NULL)
    {
        return (char *)"[骑士]";
    }
    else if (strstr(类名, "girl") != NULL)
    {
        return (char *)"[小女孩]";
    }
    else
    {
        return (char *)类名;
    }
}

static char *getboss(const char *类名)
{
    if (strstr(类名, "joseph") != NULL)
    {
        return (char *)"[约瑟夫]";
    }
    else if (strstr(类名, "banshee") != NULL)
    {
        return (char *)"[红蝶]";
    }
    else if (strstr(类名, "skill_stz") != NULL || strstr(类名, "grocer") != NULL)
    {
        return (char *)"[杂货商]";
    }
    else if (strstr(类名, "sxwd") != NULL)
    {
        return (char *)"[小丑]";
    }
    else if (strstr(类名, "chuanhuo") != NULL)
    {
        return (char *)"[厂长残火]";
    }
    else if (strstr(类名, "redqueen") != NULL)
    {
        return (char *)"[红夫人]";
    }
    else if (strstr(类名, "dm65_butcher_ll") != NULL)
    {
        return (char *)"[鹿头]";
    }
    else if (strstr(类名, "ripper") != NULL)
    {
        return (char *)"[杰克]";
    }
    else if (strstr(类名, "spider") != NULL)
    {
        return (char *)"[蜘蛛]";
    }
    else if (strstr(类名, "lizard") != NULL)
    {
        return (char *)"[孽蜥]";
    }
    else if (strstr(类名, "hastur") != NULL)
    {
        return (char *)"[黄衣之主]";
    }
    else if (strstr(类名, "wuchang") != NULL)
    {
        if (strstr(类名, "white") != NULL)
        {
            return (char *)"[白无常]";
        }
        else if ((类名, "black") != NULL)
        {
            return (char *)"[黑无常]";
        }
    }
    else if (strstr(类名, "burke") != NULL)
    {
        return (char *)"[疯眼]";
    }
    else if (strstr(类名, "yidhra_xintu") != NULL)
    {
        return (char *)"[原生信徒]";
    }
    else if (strstr(类名, "yidhra") != NULL)
    {
        return (char *)"[梦之女巫]";
    }
    else if (strstr(类名, "boy_shu") != NULL)
    {
        return (char *)"[安息松]";
    }
    else if (strstr(类名, "boy") != NULL)
    {
        return (char *)"[爱哭鬼]";
    }
    else if (strstr(类名, "bomber") != NULL || strstr(类名, "bonbon") != NULL)
    {
        return (char *)"[26号守卫]";
    }
    else if (strstr(类名, "messager") != NULL)
    {
        return (char *)"[噩梦]";
    }
    else if (strstr(类名, "joan") != NULL)
    {
        return (char *)"[使徒]";
    }
    else if (strstr(类名, "paganini") != NULL)
    {
        return (char *)"[小提琴家]";
    }
    else if (strstr(类名, "sculptor") != NULL)
    {
        return (char *)"[雕刻家]";
    }
    else if (strstr(类名, "polun") != NULL)
    {
        return (char *)"[破轮]";
    }
    else if (strstr(类名, "frank") != NULL)
    {
        return (char *)"[博士]";
    }
    else if (strstr(类名, "yunv") != NULL)
    {
        return (char *)"[渔女]";
    }
    else if (strstr(类名, "laxiang") != NULL || strstr(类名, "wax") != NULL)
    {
        return (char *)"[蜡像师]";
    }
    else if (strstr(类名, "lady") != NULL)
    {
        return (char *)"[记录员]";
    }
    else if (strstr(类名, "ithaqua") != NULL)
    {
        return (char *)"[守夜人]";
    }
    else if (strstr(类名, "famingjia") != NULL || strstr(类名, "hermit") != NULL)
    {
        return (char *)"[隐士]";
    }
    else if (strstr(类名, "goat") != NULL)
    {
        return (char *)"[歌剧演员]";
    }
    else if (strstr(类名, "spkantan") != NULL)
    {
        return (char *)"[愚人金]";
    }
    else if (strstr(类名, "yith") != NULL)
    {
        return (char *)"[时空之影]";
    }
    else if (strstr(类名, "butcher") != NULL)
    {
        return (char *)"[厂长]";
    }
    else if (strstr(类名, "space") != NULL)
    {
        return (char *)"[坡脚羊]";
    }
    else if (strstr(类名, "spzaji") != NULL)
    {
        return (char *)"[喧嚣]";
    }
    else
    {
        return (char *)类名;
    }
}

static char *getprop(const char *类名)
{
    if (strstr(类名, "h55_pendant_inject") != NULL)
    {
        return (char *)"[镇静剂]";
    }
    else if (strstr(类名, "h55_pendant_moshubang") != NULL)
    {
        return (char *)"[魔术棒]";
    }
    else if (strstr(类名, "h55_pendant_flaregun") != NULL)
    {
        return (char *)"[信号枪]";
    }
    else if (strstr(类名, "h55_pendant_huzhou") != NULL)
    {
        return (char *)"[护肘]";
    }
    else if (strstr(类名, "h55_pendant_map") != NULL)
    {
        return (char *)"[地图]";
    }
    else if (strstr(类名, "h55_pendant_book") != NULL)
    {
        return (char *)"[书]";
    }
    else if (strstr(类名, "h55_pendant_gjx") != NULL)
    {
        return (char *)"[工具箱]";
    }
    else if (strstr(类名, "h55_pendant_glim") != NULL)
    {
        return (char *)"[手电筒]";
    }
    else if (strstr(类名, "h55_pendant_xiangshuiping") != NULL)
    {
        return (char *)"[忘忧之香]";
    }
    else if (strstr(类名, "h55_pendant_controller") != NULL)
    {
        return (char *)"[遥控器]";
    }
    else if (strstr(类名, "h55_pendant_football") != NULL)
    {
        return (char *)"[橄榄球]";
    }
    else if (strstr(类名, "h55_pendant_huaibiao") != NULL)
    {
        return (char *)"[怀表]";
    }
    else if (strstr(类名, "h55_pendant_puppet") != NULL)
    {
        return (char *)"[厂长傀儡]";
    }
    else if (strstr(类名, "h55_pendant_tower") != NULL)
    {
        return (char *)"[窥视者]";
    }
    else if (strstr(类名, "h55_pendant_patro") != NULL)
    {
        return (char *)"[巡视者]";
    }
}

static char *getscene(const char *类名)
{
    if (strstr(类名, "dm65_scene_prop_30") != NULL)
    {
        return (char *)"大门";
    }
    else if (strstr(类名, "dm65_scene_prop_76") != NULL)
    {
        return (char *)"地窖";
    }
    else if (strstr(类名, "dm65_scene_gallow") != NULL)
    {
        return (char *)"狂欢之椅";
    }
    else if (strstr(类名, "dm65_scene_prop_01") != NULL)
    {
        return (char *)"道具箱";
    }
    else if (strstr(类名, "masbox01") != NULL)
    {
        return (char *)"道具箱";
    }
    else if (strstr(类名, "woodplane001") != NULL)
    {
        return (char *)"板子";
    }
    else if (strstr(类名, "dm65_scene_sender_01") != NULL)
    {
        return (char *)"电机";
    }
}
