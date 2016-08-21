use [rpg]
GO

--drop table dmkStats
create table dmkStats(
	Id int identity(1, 1), 
	Name varchar(32) not null, 
	constraint PK_dmkStats primary key(Id),
	constraint UQ_dmkStats_Name unique (Name)
)
GO

-- drop table dmkPlayer
create table dmkPlayer(
	Id int identity(1, 1), 
	Name varchar(16) not null, 
	Password varchar(64) not null, 
	PasswordSalt varchar(64) not null,
	LastLoggedIn datetime null,
	constraint PK_dmkPlayer primary key(Id),
	constraint UQ_dmkPlayer_Name unique (Name)
)
GO

-- drop table dmkPlayerStats
create table dmkPlayerStats(PlayerId int not null, 
							StatId int not null, 
							Experience int not null, 
							foreign key (PlayerId) references dmkPlayer(Id),
							foreign key (StatId) references dmkStats(Id),
							constraint UQ_dmkPlayerStats_PlayerId_StatId unique (PlayerId, StatId))
GO

-- drop view dmkPlayerStatsView
create view dmkPlayerStatsView as
with cte as (
select 
	dmkPlayerStats.PlayerId,
	dmkPlayerStats.StatId,
	dmkPlayer.Name as PlayerName,
	dmkStats.Name as StatName,
	dmkPlayerStats.Experience
from dmkPlayerStats with (nolock)
inner join dmkPlayer with (nolock) on dmkPlayer.Id = dmkPlayerStats.PlayerId
inner join dmkStats with (nolock) on dmkStats.Id = dmkPlayerStats.StatId
)
select PlayerId, StatId, PlayerName, StatName, Experience from cte
union all
select PlayerId, '', PlayerName, 'total', sum(Experience) from cte
group by PlayerId, PlayerName
GO

create table dmkGameSettings (-- dmkGameSettings is the world-specific data (i.e. stuff in the world is in dmkWorldData)
	Id char(6) not null,
	Description varchar(64) not null,
	Value varchar(32) not null,
	constraint PK_dmkGameSettings primary key (Id)
)
GO

create procedure dmkAddExp(@PlayerId int, @StatId int, @Experience int) as
update dmkPlayerStats set Experience = Experience + @Experience where PlayerId=@PlayerId and StatId=@StatId
GO

-- will need an image table
-- should this be split into several tables?
-- i.e. heads/bodies/legs/weapons/trees/rocks/ground
-- heads/bodies/legs would hold all the base looks as well as armour
-- these images will be sent across to the client in base64 format to be parsed