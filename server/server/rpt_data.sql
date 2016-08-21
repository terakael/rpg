-- this should be run AFTER rpg.sql (which is the schema)
use [rpg]
GO

-- truncate table dmkStats
insert into dmkStats(Name) values ('strength')
insert into dmkStats(Name) values ('accuracy')
insert into dmkStats(Name) values ('defence')
insert into dmkStats(Name) values ('agility')
insert into dmkStats(Name) values ('hitpoints')
insert into dmkStats(Name) values ('mining')
insert into dmkStats(Name) values ('smithing')
insert into dmkStats(Name) values ('magic')
-- select * from dmkStats
GO

-- truncate table dmkPlayer
insert into dmkPlayer(Name, Password, PasswordSalt) values ('dmk', 'Password12', 'abcdef012345')
-- select * from dmkPlayer
GO

-- truncate table dmkPlayerStats
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 1, 0)-- str
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 2, 0)-- att
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 3, 0)-- def
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 4, 0)-- agil
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 5, 100) --hp
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 6, 0)-- mining
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 7, 0)-- smithing
insert into dmkPlayerStats (PlayerId, StatId, Experience) values (1, 8, 0)-- magic
-- select * from dmkPlayerStats
GO

-- select * from dmkPlayerStatsView

-- truncate table dmkGameSettings
-- notes:
-- width of the camera will be cvsw - hudw
insert into dmkGameSettings (Id, Description, Value) values ('w', 'world width (in tiles)', '3000')
insert into dmkGameSettings (Id, Description, Value) values ('h', 'world height (in tiles)', '3000')
insert into dmkGameSettings (Id, Description, Value) values ('tilew', 'tile width (in pixels)', '32')
insert into dmkGameSettings (Id, Description, Value) values ('tileh', 'tile height (in pixels)', '32')
insert into dmkGameSettings (Id, Description, Value) values ('maxplr', 'max players', '1000')
insert into dmkGameSettings (Id, Description, Value) values ('fps', 'frames per second', '60')
insert into dmkGameSettings (Id, Description, Value) values ('cvsw', 'canvas width', '800')
insert into dmkGameSettings (Id, Description, Value) values ('cvsh', 'canvas height', '600')
insert into dmkGameSettings (Id, Description, Value) values ('hudw', 'hud width', '250')--hudh == cvsh
insert into dmkGameSettings (Id, Description, Value) values ('msgbc', 'message broadcast rate (seconds)', '0.6')
GO
-- select * from dmkGameSettings