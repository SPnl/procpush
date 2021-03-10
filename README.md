# Documentatie datamanipulaties procpush module #

De SP stuurt contactgegevens door naar Procurios. Daarvoor hebben we de procpush module ontwikkeld. Dit functioneert als een primitieve API waar we (ruwe) contactgegevens van verschillende bronnen heen kunnen sturen. Voor doemee.sp.nl komt deze data van de spwebformsync module die de data weer uit webformulier inzendingen haalt. Maar dit kunnen ook andere bronnen zijn (app ontwikkelaar). De data die bij deze module binnenkomt beschouwen we als ruwe data,  vandaar dat we de data zo goed mogelijk valideren en opschonen en geschikt maken voor Procurios API in deze module. Dat voorkomt dat je op verschillende plekken onnodig dezelfde validaties gaat ontwikkelen. Deze documentatie beschrijft de data die de procpush module ontvangt, en de manipulaties die gedaan worden om de data geschikt te maken voor de API van Procurios. Als de data geschikt is gemaakt voor de API van procurios wordt middels de procapi module e.e.a. naar Procurios gestuurd.

## Binnenkomende data structuur ##

De binnenkomende data wordt door de procpush_push_contact() functie opgevangen. De data die deze ontvangt heeft de vorm van een array. Zie hieronder voor de beschikbare data velden. In de code komt ook nog het contactnummer veld voor, deze heb ik in deze documentatie weggelaten omdat die (voorlopig) voor de nieuwe implementatie niet nodig is.

***Binnenkomende velden procpush module***

```
Array
(
    [relation_id]
    [name]
    [first_name]
    [middle_name]
    [last_name]
    [email]
    [phone]
    [fixed_phone]
    [mobile_phone]
    [fixed_phone_work]
    [mobile_phone_work]
    [street]
    [house_number_and_addition]
    [house_number]
    [house_number_addition]
    [postal_code]
    [locality]
    [country]
    [overwrite]
    [selections]
    [sp_active]
    [sp_work_and_int]
    [privacy]
    [sp_news_subscription]
)
```

***Voorbeelden binnenkomende data***

```
Array
(
    [name] => Bart van Bregen
    [email] => bartvanbregen@gmail.com
    [phone] => 0647382956
    [street] => Bregenstraat
    [house_number_and_addition] => 47A
    [postal_code] => 3434BB
    [selections] => Array
        (
            [add] => Array
                (
                    29
                    30
                )
            [remove] => Array
                (
                    24
                )
        )
    [sp_news_subscription] => 1
)
```

```
Array
(
    [relation_id] => 3458765
    [postal_code] => 3434 BB
    [house_number] => 47
    [house_number_addition] => A
    [sp_active] => Array
        (
            [active] => incidenteel
            [activities] => Array
                (
                    afdelingsbestuur
                    klussen
                )

        )

    [sp_work_and_int] => Array
        (
            [main_task] => Array
                (
                    betaald werk
                )

            [industrial_sector] => bouw of installatie
            [occupational_group] => bouw of installatie
        )

    [privacy] => Array
        (
            [do_not_phone] => 1
            [do_not_sms] => 1
        )

)
```

```
Array
(
    [relation_id] => 3458765
    [postal_code] => 3434 BB
    [fixed_phone_work] => [remove:0325364745]
    [mobile_phone_work] => [remove:0645362718]
    [overwrite] => 1
)
```

***Toelichting telefoonveld***

De telefoonnummer velden kunnen een waarde [remove:0628329832] hebben. De bedoeling is dan dat het telefoonnummer in Procurios verwijderd wordt. Dit is een lelijke oplossing. Dit zou bij een nieuwe opzet beter kunnen worden opgelost zoals bij de selectie data structuur:

```
Array
(
    [relation_id] => 3458765
    [fixed_phone] => array

​        (

​            [remove] =>   0325364745

​            [add] => 04554345345

​        )

​    [mobile_phone_work] => array

​        (

​            [remove] =>  0645362718

​        )
​    [overwrite] => 1
)
```

***Toelichting overwrite veld***

Alleen als deze optie waar is worden bestaande contactgegevens (naam, tel, email, adres) van leden overschreven door de nieuwe data in Procurios.

***Toelichting sp_active veld***

Met dit veld kan worden aangeven in hoeverre, en waarmee een relatie actief is bij de SP.

Mogelijke waarden

* active (single value)
    * structureel
    * incidenteel
    * niet
* activities (multivalue)
    * administratief werk
    * afdelingsbestuur
    * belteam
    * folderen
    * hulpdienst
    * klussen
    * ledenbezoek
    * markt
    * media/website
    * campagnes/acties
    * plakken
    * politieke basisvorming
    * postverzending
    * raadsfractie
    * rijden
    * rood
    * specifieke kunde
    * tribune rondbrengen
    * wijkcontactpersoon
    * kloppen

***Toelichting sp_work_and_int veld***

Met dit veld kan van relaties worden bijgehouden wat hun werkzaamheden zijn.

In de code zal nog de categorie 'membership' terug te vinden zijn. Die heb ik hier weggelaten. We mogen vanwege privacy wetgeving niet het lidmaatschap van vakbonden e.d. bijhouden.

Mogelijke waarden. 
* main_task (multivalue)
    * betaald werk
    * freelance/zelfstandige
    * student/scholier
    * huisman/vrouw
    * arbeidsongeschikt
    * gepensioneerd
    * vrijwilliger
    * werkloos
* industrial_sector (single value)
    * agrarische sector
    * bouw of installatie
    * cultuur, sport, vrije tijd
    * energie, delfstoffen, milieu
    * gezondheidszorg
    * grafische sector, reclame
    * handel, verhuur, reparatie
    * horeca, catering, verblijfsrecreatie
    * ict, telecommunicatie
    * industrie, productiebedrijf
    * onderwijs, universiteit, training
    * onderzoek, keuring en certificering
    * overheid (gemeente, provincie, rijk, waterschap)
    * transport en logistiek
    * vereniging, stichting, koepelorganisatie
    * verzorging, welzijn, kinderopvang
    * zakelijke dienstverlening, bank, verzekeringsbedrijf
    * anders
* occupational_group (single value)
    * administratief beroep
    * adviseur, consulent, consultant, voorlichter
    * 'agrarisch beroep
    * 'bank, verzekering, belasting, accountant
    * 'beveiliging, toezicht, politie, defensie
    * 'bouw of installatie
    * 'docent, trainer, onderzoeker
    * 'elektrotechnicus, -monteur, elektricien
    * 'grafisch, journalistiek, media, pr, marketing
    * 'horeca, catering
    * 'ict-beroep
    * 'inkoper, verkoper, sales
    * 'logistiek, transport, planner
    * 'maatschappelijk werk, welzijn
    * 'medisch, paramedisch, verzorgend, huishoudelijk
    * 'personeelswerk
    * 'productiemedewerker
    * 'secretaresse, secretaris
    * 'staf, management, juridisch
    * winkel

***Toelichting privacy veld***

Mogelijke waarden.

*  do_not_mail (boolean) (Geen post)
*   do_not_email (boolean)
*   do_not_phone (boolean)
*   do_not_sms (boolean)
*   is_opt_out (boolean) (Geen massamails ontvangen)



## Data manipulaties ##

De binnenkomende data ondergaat een aantal manipulaties alvorens deze naar Procurios wordt gestuurd.

* Als een naam(deel) eindigd op -test wordt er geen data naar Procurios gestuurd. De mensen die bij ons formulieren inrichten willen vaak testen of mails goed verzonden worden e.d. in de webformulieren zonder dat de test data naar Procurios gaat.

* Als een telefoonnummer veld een 'remove bevat': [remove:0612345678] dan wordt het telefoonnummer 060000000 als waarde van het betreffende veld ingevuld. De API van Procurios weet dan dat het telefoonnummer dat er in Procurios staat voor het betreffende veld verwijderd moet worden. De oplossing met [remove:...] is lelijk zoals boven reeds aangegeven. Het is wenselijk dat binnenkomende data voor telefoon velden een array structuur krijgt met remove en add als opties met als waarde een telefoonnr zoals in de toelichting op het veld hierboven aangegeven. Dan moet natuurlijk nog steeds de telefoonnr. veld waarde vervangen worden door 0600000000 als het nummer verwijderd, of door het telefoonnummer als het toegevoegd moet.
* Spaties aan het begin of einde van een string type veld worden verwijderd.
* Dubbele spaties in een string type veld worden verwijderd.
* Emojis in strings worden verwijderd.
* De volgende karakters ?!#$%^*{}[\];:<> worden uit strings verwijderd.
* Als het voornaam en achternaam veld leeg zijn en het naam veld niet, dan wordt het naam veld gesplitst in voornaam, tussenvoegsel en achternaam, en het naam veld wordt verwijderd. De reden hiervoor is dat Procurios API geen ongesplitst naam veld accepteert, en wij graag ongesplitste naam velden gebruiken in onze formulieren.
* Als het huisnummer inclusief toevoeging veld niet leeg is, en het huisnummer veld is leeg, dan wordt het huisnummer inclusief toevoeging veld gesplitst in een huisnummer en huisnummer toevoeging veld en het huisnummer inclusief toevoeging veld wordt verwijderd.
* Als het telefoonnummer veld niet leeg is, dan wordt bepaald of het om een mobiel of vast telefoonnummer gaat, en het betreffende veld wordt dan gevuld, het telefoonnummer veld wordt verwijderd.
* Het relation_id veld wordt gecontroleerd of dat een valide integer nummer is, is dat niet zo, dan wordt het veld verwijderd.
* Van de voornaam of voornamen en achternaam of achternamen wordt de eerste letter een hoofdletter gemaakt, en de rest kleine letters.
* Tussenvoegsels worden kleine letters gemaakt.
* Er wordt gecontroleerd het een geldig e-mailadres betreft qua syntax, is dat niet het geval wordt het e-mailveld verwijderd.
* Er wordt gekeken of het een Nederlands telefoonnummer betreft. Als dat zo is dan:
    * Internationale voorvoegsel Nederlands nummer wordt verwijderd: 310612345678 => 0612345678, 31612345678 => 0612345678,  +31612345678 => 0612345678,  00310612345678 => 0612345678.
    * Telefoonnummers waarvan de 0 aan het begin ontbreekt worden gerepareerd: 612345678 => 0612345678. De reden hiervoor is dat als data uit een spreadsheet komt en er een nummeriek ipv. een tekstveld wordt gebruikt voor telefoonnummers, nullen aan het begin ontbreken.
* Straatnaam krijgt begin hoofdletter.
* Van een plaatsnaam worden alle letters hoofdletters.
* Van selecties veld wordt gecontroleerd of deze juiste structuur heeft.
* Van SP Actief veld wordt gecontroleerd of deze juiste structuur heeft.
* Van SP Werk en Interesses veld wordt gecontroleerd of deze juiste structuur heeft.
* Het overschrijven veld krijgt TRUE als waarde als deze niet leeg is, en anders FALSE.
* Check of de data die over is genoeg contactgegevens bevat.
    * Als er een relatie id is, dan ok.
    * Als de achternaam niet leeg is en niet gelijk is aan '.' of als de voornaam niet leeg is en er is daarnaast ook nog een e-mailadres, een volledig adres of vast telefoonnummer of mobiel telefoonnummer, dan ok.
* Als er geen achternaam is, maak de achternaam dan '.'. Dit is nodig voor de API van Procurios, die accepteert geen lege achternaam velden.
* Als er geen e-mailadres is maak het e-mailadres dan 'punt@sp.nl'. Dit is nodig voor de API van Procurios, die accpeteerd geen leeg e-mailadres veld.
* Als er adresgegevens zijn, maar het land veld is leeg, stel het land dan in op 'NL'.
* Maak nu de data geschikt voor de Procurios API. De veldnamen moeten gewijzigd worden naar veldnamen die de Procurios API aan kan, en de data van de velden moet eventueel geschikt gemaakt worden zodat de Procurios API deze kan verwerken. De veldnamen in de Procurios API kunnen verschillen per Procurios omgeving (staging, live) vandaar dat deze mapping niet hard coded kan. Hoe de verschillende velden heten, en welke data ze verwachten is terug te vinden via een Procurios API call. Hoe dat werkt is hier te vinden: http://api.sp-staging2.procurios.site/api_documentation/registration/profile. In de procpush module wordt de mapping van binnenkomende velden naar Procurios API velden ingesteld via /admin/config/sp/procpush.
* De data is nu geschikt om naar de Procurios registratie API te sturen. Dat gaat via de procapi module: procapi_registration_push_data_object($data_object).
* Een relatie vervolgens nog toevoegen aan, of verwijderen uit selecties gaat via procapi_selection_add_relations() en procapi_selection_remove_relations().
