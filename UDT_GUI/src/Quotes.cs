using System.Security.Cryptography;


namespace Uber.DemoTools
{
    public static class Quotes
    {
        public static string GetRandomQuote()
        {
            var generator = RNGCryptoServiceProvider.Create();
            var bytes = new byte[1];
            generator.GetBytes(bytes);
            var index = (int)bytes[0] % _quotes.Length;

            return _quotes[index];
        }

        private static string[] _quotes = new string[]
        {
            "ugh I'm going to bad sorry man :< (c) Terifire",
            "FYCJ IU CANT TYPE (c) Terifire",
            "maybe they were could (c) Terifire",
            "maybe I can drink my shaft sorrows away (c) Terifire",
            "And people would someone get passed the password screen and snoop around on my computer. (c) Terifire",
            "It's 1:55, expect met o make typoes =) (c) Terifire",
            "In the last month I have produced so much luck I could shit a leprechaun (c) Terifire",
            "no I'm trying to create an nmap script so I can brute force a porn site's http auth (c) Terifire",
            "man brute-forcing ironically is not the best way to retrieve porn site passwords (c) Terifire",
            "He's getting a load of my shaft for christmas (c) Terifire",
            "myT I am very impress with your tool (c) Terifire",
            "im gonna make so many airrockets nasa will declare bankrupcytsty (c) ovol1ty",
            "rest gonna be cuted (c) gogetto",
            "can i watch you doing it ? (c) gogetto",
            "O kjust wann aplslep (c) gaiia",
            "you make have grown your own fingers.. but sometimes you need to curse them and threaten them.. whatevah God do what he want (c) nerfquark",
            "myT I am very impress with your tool (c) Terifire",
            "He's getting a load of my shaft for christmas (c) Terifire",
            "can i watch you doing it ? (c) gogetto",
            "chakled litel boll (c) v3nom",
            "so drinked (c) Rizla-",
            "id rather eat ur mothers shit after shes been on a shit diet for a month (c) pcb",
            "there was a quato related james bond (c) TheDynamite",
            "this frag was sponsored by luck (c) Ultrameta",
            "fuck so many binds (c) 5immy",
            "we had wii and we got bad resoults in bed :D so i sold it haha (c) FcuKFcuKFuKeT",
            "neither of you take one frag from me in a duel (c) Spetz",
            "you peace of shit take other model (c) QiGonG",
            "u only got one router in your country? (c) ilu",
            "hi im myt, half of my keyboard is binded to pcb quotes (c) pcb",
            "tdm is all about poland and cpm27 (c) aab",
            "ye i finally got my own quote (c) damn-",
            "i dont need your help making me look like an idiot (c) pcb",
            "why hi ban me i ask sub:( (c) bullet",
            "WAXEDDDDDDDDDDDDDDDDDDDD (c) pejnisson",
            "PAYJETIC TEAM (c) Falko",
            "DOODR, 1SEX (c) xero",
            "niceninciencienceinceinceicnnice (c) DDK",
            "in my village, man and woman looks the same (c) mih",
            "i was born to def (c) TheDynamite",
            "ill send you a mold of my ass (c) pcb",
            "im danish im born like that (c) pcb",
            "how not to rage if you can see chernobyl from your window (c) vicek",
            "fuck me up (c) Pushpabon",
            "cant talking with pll like u (c) olszanek",
            "thank god you dont quote me (c) guh4rd",
            "get the fuck on with it (c) mistralol",
            "don't funny (c) lennox",
            "sorry, my englich is easy (c) waitze",
            "im riding the dog (c) pcb",
            "AGHAGHAGHAGHAGHAGHAGHAGAHGAHGA (c) AHXNXA",
            "i thought norz n nzro were 2 diff ppl (c) pcb",
            "!add TDM (c) Pushpabon",
            "hands too cold to rail (c) Spetz",
            "cpma the modern man's chess (c) panic",
            "i am a total moron (c) sanchi",
            "im good now cause i changed model and shit (c) panic",
            "let me think (c) panic",
            "fuyck foff (c) astro",
            "sry i can play (c) Rizla-",
            "ethad is the son of ozzy osbourne (c) Rizla-",
            "Rizla- = ebony who like anal sex (c) Rizla-",
            "i'm dirtier than an illegal immigrant after a threesome with rizla's sisters (c) shadow",
            "Well I shared some of my kugelns with them:P (c) Terifire",
            "grate game (c) supercruel",
            "balanced rape (c) myT",
            "+ = hiv pazitive (c) killa_",
            "myT puts the C in CP em (c) mercur1",
            "lets cum to myt (c) gogetto",
            "i know i dunno (c) gogetto",
            "Glad you enjoyed it as much as I did (c) mejtisson",
            "Going to macdonalds for a salad is like going to hug a whore with aids. (c) shadow",
            "and now i god (c) gogetto",
            "maybe something is wrong with the flux capacitor (c) Terifire",
            "im sick of your mega luck (c) Nzr0",
            "proof me wrong (c) raeg",
            "i so fucking rape with below 80 ping (c) Raist",
            "dunno i always had a soft hard for ca (c) Raist",
            "ass much as i care abaut you (c) brithey",
            "i was born to def (c) TheDynamite",
            "cOwabanga <3 (c) mih8r",
            "sorry, my englich is eazy (c) waitze",
            "I like run run run! And stuff like that. (c) Cooller",
            "I am talking it to you (c) Cooller",
            "le lightsaber, terriiiiiiiiiible (c) Rizla-",
            "I have no life. (c) eThaD",
            "I ate so much I move like a VQ3er :/ (c) myT",
            "i men (c) gog",
            "Christiaan: just came with me? (c) gog",
            "You know gog I sometimes wonder what the space-time continuum looks like on your side (c) myT",
            "I was a video once of a bridge in switzerland (c) Terifire"
        };
    }
}