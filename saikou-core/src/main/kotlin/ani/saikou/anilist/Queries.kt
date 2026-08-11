package ani.saikou.anilist

/**
 * The GraphQL documents. Kept as one flat object so the field selections stay visible —
 * every field here has to be paid for in response size, and the UI only renders these.
 */
object Queries {
    private const val MEDIA_FIELDS = """
        id
        idMal
        title { romaji english native userPreferred }
        coverImage { extraLarge large color }
        bannerImage
        description(asHtml: false)
        episodes
        duration
        status
        format
        season
        seasonYear
        averageScore
        meanScore
        popularity
        genres
        isAdult
        nextAiringEpisode { episode airingAt timeUntilAiring }
        mediaListEntry { id status progress score(format: POINT_10) }
    """

    val TRENDING = """
        query (${'$'}page: Int, ${'$'}perPage: Int) {
            Page(page: ${'$'}page, perPage: ${'$'}perPage) {
                pageInfo { hasNextPage currentPage }
                media(type: ANIME, sort: TRENDING_DESC, isAdult: false) { $MEDIA_FIELDS }
            }
        }
    """

    val POPULAR_THIS_SEASON = """
        query (${'$'}page: Int, ${'$'}perPage: Int, ${'$'}season: MediaSeason, ${'$'}seasonYear: Int) {
            Page(page: ${'$'}page, perPage: ${'$'}perPage) {
                pageInfo { hasNextPage currentPage }
                media(type: ANIME, season: ${'$'}season, seasonYear: ${'$'}seasonYear,
                      sort: POPULARITY_DESC, isAdult: false) { $MEDIA_FIELDS }
            }
        }
    """

    val SEARCH = """
        query (${'$'}search: String, ${'$'}page: Int, ${'$'}perPage: Int) {
            Page(page: ${'$'}page, perPage: ${'$'}perPage) {
                pageInfo { hasNextPage currentPage }
                media(type: ANIME, search: ${'$'}search, sort: SEARCH_MATCH, isAdult: false) { $MEDIA_FIELDS }
            }
        }
    """

    /**
     * The Browse screen's one query. Every filter is nullable, and AniList skips a
     * null argument entirely, so the same document serves "everything, by popularity"
     * and "action + drama, TV, this season, by score".
     */
    val BROWSE = """
        query (${'$'}page: Int, ${'$'}perPage: Int, ${'$'}search: String, ${'$'}genres: [String],
               ${'$'}sort: [MediaSort], ${'$'}format: MediaFormat, ${'$'}season: MediaSeason,
               ${'$'}seasonYear: Int, ${'$'}status: MediaStatus) {
            Page(page: ${'$'}page, perPage: ${'$'}perPage) {
                pageInfo { hasNextPage currentPage }
                media(type: ANIME, search: ${'$'}search, genre_in: ${'$'}genres, sort: ${'$'}sort,
                      format: ${'$'}format, season: ${'$'}season, seasonYear: ${'$'}seasonYear,
                      status: ${'$'}status, isAdult: false) { $MEDIA_FIELDS }
            }
        }
    """

    val GENRES = """
        query { GenreCollection }
    """

    /**
     * The week's airing schedule. One flat, time-sorted list; the UI buckets it into days
     * in the local timezone, which the server cannot do for us.
     */
    val AIRING = """
        query (${'$'}start: Int, ${'$'}end: Int, ${'$'}page: Int, ${'$'}perPage: Int) {
            Page(page: ${'$'}page, perPage: ${'$'}perPage) {
                pageInfo { hasNextPage currentPage }
                airingSchedules(airingAt_greater: ${'$'}start, airingAt_lesser: ${'$'}end,
                                sort: TIME) {
                    id
                    episode
                    airingAt
                    media { $MEDIA_FIELDS }
                }
            }
        }
    """

    val MEDIA = """
        query (${'$'}id: Int) {
            Media(id: ${'$'}id, type: ANIME) {
                $MEDIA_FIELDS
                studios(isMain: true) { nodes { name } }
                relations { edges { relationType node { id title { userPreferred } coverImage { large } type } } }
                recommendations(sort: RATING_DESC, perPage: 12) {
                    nodes { mediaRecommendation { id title { userPreferred } coverImage { large } } }
                }
            }
        }
    """

    val VIEWER = """
        query {
            Viewer {
                id
                name
                avatar { large }
                bannerImage
                options { displayAdultContent titleLanguage }
                statistics { anime { count episodesWatched minutesWatched } }
            }
        }
    """

    /**
     * The watch list. `CURRENT` and `REPEATING` together are what "Continue watching"
     * means to a user — a rewatch is still something they are actively watching.
     */
    val USER_LIST = """
        query (${'$'}userId: Int, ${'$'}status: [MediaListStatus]) {
            MediaListCollection(userId: ${'$'}userId, type: ANIME, status_in: ${'$'}status,
                                sort: UPDATED_TIME_DESC) {
                lists {
                    name
                    status
                    entries {
                        id
                        status
                        progress
                        score(format: POINT_10)
                        updatedAt
                        media { $MEDIA_FIELDS }
                    }
                }
            }
        }
    """

    val SAVE_PROGRESS = """
        mutation (${'$'}mediaId: Int, ${'$'}progress: Int, ${'$'}status: MediaListStatus) {
            SaveMediaListEntry(mediaId: ${'$'}mediaId, progress: ${'$'}progress, status: ${'$'}status) {
                id
                status
                progress
            }
        }
    """

    val SAVE_SCORE = """
        mutation (${'$'}mediaId: Int, ${'$'}score: Float) {
            SaveMediaListEntry(mediaId: ${'$'}mediaId, scoreRaw: ${'$'}score) { id score }
        }
    """
}
