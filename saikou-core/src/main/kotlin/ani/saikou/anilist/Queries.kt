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
