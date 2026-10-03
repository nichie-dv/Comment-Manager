#pragma once

#include <vector>
#include <string>

#include "Structures.hpp"

//Bad words :o
inline const std::vector<BannedWord> DefaultWordlist = makeWordlist({
    //porn
    "p*rn", "pr*n", "ph*rn", "p*rno", "p*rnography", "pr*nography", "p*rnhub", "pr*nhub",
    "pr*n", "phr*n", "ph*rn",

    //onlyfans / nsfw
    "*nlyf*ns",
    "n*fw",

    //profanity
    "f*ck", "fck", "fuk", "fuq",
    "sh*t", "shiit", "sht",
    "b*tch", "bi*ch", "bich",
    "*sshole", "a**hole", "ashole",
    "d*ck", "dik", "d!k",
    "p*ssy", "pu**y", "puss*",
    "c*nt", "kunt",
    "c*ck", "cok", "kock",
    "wh*re",
    "sl*t", "*lut",
    "r*pe",

    //sexual
    "s*x", "s*xx", "s*xy", "s*xi", "s*xxi",
    "nude", "n**de", "n*des",
    "nak*d", "n*ked",
    "b**bs",
    "t*ts",
    "c*m", "cumm",
    "d*ldo",
    "h*nt*i",
    "x*x",

    //slang
    "stfu", "wtf", "lmfao", "lmao", "kys",

    //spam / scams
    "sc*m", "scammm",
    "freerobux", "freenitro", "freegems", "freediamonds",
    "clickhere", "clickthislink",
    "dmme", "dmfor", "messageme",
    "buyfollowers", "freefollowers", "freelikes", "freesubscribers",
    "cryptogiveaway", "airdrop", "walletconnect",
    "doubleyourmoney", "makemoneyfast", "easymoney", "investmentopportunity",
    "bitcoin", "ethereum", "telegram", "whatsapp", "discordgg", "bitly", "tinyurl",

    //"adult"
    "adultcontent", "adult", "explicit", "18plus", "cp",
});

inline bool isStringBanned(std::string string) {
    return true;
};