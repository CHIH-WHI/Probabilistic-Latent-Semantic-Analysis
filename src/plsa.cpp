#include "..\General library\General library.h"

int main()
{
	constexpr std::string_view DocumentsDirectory = R"(..\ntust-ir-2020_hw4_v2\docs)";
	constexpr std::string_view QueriesDirectory = R"(..\ntust-ir-2020_hw4_v2\queries)";
	constexpr std::string_view OutputHeader = R"(Query,RetrievedDocuments)";
	constexpr std::string_view OutputPathExtension = R"(.txt)";
	constexpr std::size_t OutputDocumentsCount = 1000;

	constexpr double DocumentAlpha = 0.5;
	constexpr double DocumentBeta = 0.7;
	constexpr double DocumentGamma = 0.0;
	constexpr double DocumentBetaPrime = 0.3;
	constexpr double DocumentGammaPrime = 0.3;

	constexpr std::size_t PLSASampleCount = 1;
	constexpr std::size_t TopicCount = 32;
	constexpr std::size_t ExpectationMaximizationCount = 32;
	constexpr bool UseRandomSeed = false;

	//unsigned int ThreadCount = std::thread::hardware_concurrency();
	unsigned int ThreadCount = 6;

	// Start

	struct LocalTime GetLocalTime;
	std::cout.imbue(std::locale(""));

	std::vector<std::chrono::system_clock::time_point> TimeVector{ std::chrono::system_clock::now() };
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Start" << std::endl;

	// Get File Path

	std::vector<std::filesystem::path> QueriesPath{ std::filesystem::directory_iterator{ QueriesDirectory }, std::filesystem::directory_iterator{} };
	std::vector<std::filesystem::path> DocumentsPath{ std::filesystem::directory_iterator{ DocumentsDirectory }, std::filesystem::directory_iterator{} };

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Get File Path" << std::endl;

	// Get File Context

	std::vector<std::string> QueriesContext(std::size(QueriesPath));
	std::transform(std::begin(QueriesPath), std::end(QueriesPath), std::begin(QueriesContext), ReadFile<typename decltype(QueriesPath)::const_reference>);

	std::vector<std::string> DocumentsContext(std::size(DocumentsPath));
	std::transform(std::begin(DocumentsPath), std::end(DocumentsPath), std::begin(DocumentsContext), ReadFile<typename decltype(DocumentsPath)::const_reference>);

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Get File Context" << std::endl;

	// Get File Word Count Map

	using MapType = std::map<std::string, std::size_t>;

	std::vector<MapType> QueriesWordCountMapSequence(std::size(QueriesContext));
	std::transform(std::begin(QueriesContext), std::end(QueriesContext), std::begin(QueriesWordCountMapSequence), std::begin(QueriesWordCountMapSequence), GetWordCountMap<std::string, MapType&>);

	std::vector<MapType> DocumentsWordCountMapSequence(std::size(DocumentsContext));
	std::transform(std::begin(DocumentsContext), std::end(DocumentsContext), std::begin(DocumentsWordCountMapSequence), std::begin(DocumentsWordCountMapSequence), GetWordCountMap<std::string, MapType&>);

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Get File Word Count Map" << std::endl;

	// Get Word Index Map

	std::unordered_set<typename MapType::key_type> WordSet{};
	Flatten2Dto1D(std::begin(QueriesWordCountMapSequence), std::end(QueriesWordCountMapSequence), std::inserter(WordSet, std::end(WordSet)), std::mem_fn(&MapType::value_type::first));
	Flatten2Dto1D(std::begin(DocumentsWordCountMapSequence), std::end(DocumentsWordCountMapSequence), std::inserter(WordSet, std::end(WordSet)), std::mem_fn(&MapType::value_type::first));

	std::vector<typename MapType::key_type> WordSorted(std::size(WordSet));
	std::partial_sort_copy(std::begin(WordSet), std::end(WordSet), std::begin(WordSorted), std::end(WordSorted));

	std::size_t WordIndex{};
	std::unordered_map<typename MapType::key_type, std::size_t> WordIndexMap(std::size(WordSorted));
	std::transform(std::make_move_iterator(std::begin(WordSorted)), std::make_move_iterator(std::end(WordSorted)), std::inserter(WordIndexMap, std::end(WordIndexMap)), [&](auto&& string)
		{
			return std::make_pair(std::forward<decltype(string)>(string), WordIndex++);
		});

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Get Word Index Map" << std::endl;

	// Data Vectorization

	auto GetColumeIndex = [&](auto&& WordCountPair)
	{
		return WordIndexMap[std::forward<decltype(WordCountPair)>(WordCountPair).first];
	};

	auto GetValue = [](auto&& WordCountPair)
	{
		return static_cast<double>(std::forward<decltype(WordCountPair)>(WordCountPair).second);
	};

	std::vector<std::vector<std::size_t>> DocumentsWordIndexSequence(std::size(DocumentsWordCountMapSequence));
	Transform2Dto2D(std::begin(DocumentsWordCountMapSequence), std::end(DocumentsWordCountMapSequence), std::begin(DocumentsWordIndexSequence), GetColumeIndex);

	std::vector<std::vector<double>> DocumentsWordCountSequence(std::size(DocumentsWordCountMapSequence));
	Transform2Dto2D(std::begin(DocumentsWordCountMapSequence), std::end(DocumentsWordCountMapSequence), std::begin(DocumentsWordCountSequence), GetValue);

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds{ GetLastDifference(TimeVector) }.count() << "ns" << " Data Vectorization" << std::endl;

	//

	std::vector<double> DocumentsLengthSequence(std::size(DocumentsWordCountSequence));
	std::transform(std::begin(DocumentsWordCountSequence), std::end(DocumentsWordCountSequence), std::begin(DocumentsLengthSequence), [](auto&& FileWordCount)
		{
			return std::reduce(std::begin(std::forward<decltype(FileWordCount)>(FileWordCount)), std::end(std::forward<decltype(FileWordCount)>(FileWordCount)));
		});

	auto BackgroundLength = std::reduce(std::begin(DocumentsLengthSequence), std::end(DocumentsLengthSequence));

	auto BackgroundWordCountMap = MergeMap(std::begin(DocumentsWordCountMapSequence), std::end(DocumentsWordCountMapSequence), std::unordered_map<std::string_view, std::size_t>{}, std::plus{});

	auto TopicWordIndex = std::size_t{};
	auto BackgroundWordIndexMap = MapTransform(BackgroundWordCountMap, [&TopicWordIndex](std::size_t) { return TopicWordIndex++; });

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Data vectorization" << std::endl;

	// ---------------- Probabilistic Latent Semantic Analysis ----------------

	typedef typename decltype(QueriesWordCountMapSequence)::const_reference QueryWordCountMapConstReference;
	typedef typename decltype(DocumentsWordCountMapSequence)::const_reference DocumentWordCountMapConstReference;

	auto QueriesCount = std::size(QueriesWordCountMapSequence);
	auto DocumentsCount = std::size(DocumentsWordCountMapSequence);

	auto TopicsWordProbabilitySequence = std::vector<std::vector<double>>(TopicCount, std::vector<double>(std::size(BackgroundWordCountMap))); // Topics P(wi|Tk)
	auto DocumentsTopicProbabilitySequence = std::vector<std::vector<double>>(DocumentsCount, std::vector<double>(TopicCount)); // Documents P(Tk|dj)
	auto CountProbabilityTopicWordDocument = std::vector<std::vector<std::vector<double>>>(DocumentsCount);
	std::transform(std::begin(DocumentsWordCountMapSequence), std::end(DocumentsWordCountMapSequence), std::begin(CountProbabilityTopicWordDocument), [TopicCount](DocumentWordCountMapConstReference WordCountMap)
		{
			return std::vector<std::vector<double>>(std::size(WordCountMap), std::vector<double>(TopicCount));
		});

	auto RandomGenerator = UseRandomSeed ? std::mt19937(std::random_device()()) : std::mt19937();
	auto GenerateUniformProbabilitySequence = [&](auto& ProbabilitySequence)
	{
		GenerateProbabilitySequence(ProbabilitySequence, std::bind(std::uniform_real_distribution(), std::ref(RandomGenerator)));
	};

	auto QueriesDocumentsAccumulateIndex = std::vector<std::vector<std::size_t>>(QueriesCount, std::vector<std::size_t>(DocumentsCount, std::size_t(0)));
	for (std::size_t PLSASampleIterator = 0; PLSASampleIterator < PLSASampleCount; PLSASampleIterator++)
	{
		std::for_each(std::begin(TopicsWordProbabilitySequence), std::end(TopicsWordProbabilitySequence), GenerateUniformProbabilitySequence);

		TimeVector.push_back(std::chrono::system_clock::now());
		std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Probability Word Given Topic" << std::endl;

		std::for_each(std::begin(DocumentsTopicProbabilitySequence), std::end(DocumentsTopicProbabilitySequence), GenerateUniformProbabilitySequence);

		TimeVector.push_back(std::chrono::system_clock::now());
		std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Probability Topic Given Document" << std::endl;

		// ---------------- Expectation Maximization ----------------

		alignas(std::hardware_destructive_interference_size) std::atomic_size_t AtomicIndexFirst{};
		alignas(std::hardware_destructive_interference_size) std::atomic_size_t AtomicIndexSecond{};
		alignas(std::hardware_destructive_interference_size) std::atomic<double> AtomicLogLikelihood{};
		std::condition_variable ConditionVariable;

		std::vector<std::thread> ThreadSequence(ThreadCount);
		std::generate(std::begin(ThreadSequence), std::end(ThreadSequence), [&]
			{
				return std::thread(ThreadExpectationMaximization, ThreadCount, std::ref(AtomicIndexFirst), std::ref(AtomicIndexSecond), std::ref(ConditionVariable),
					ExpectationMaximizationCount,
					std::cref(DocumentsLengthSequence),
					std::cref(DocumentsWordIndexSequence),
					std::cref(DocumentsWordCountSequence),
					std::ref(DocumentsTopicProbabilitySequence),
					std::ref(TopicsWordProbabilitySequence),
					std::ref(CountProbabilityTopicWordDocument),
					std::ref(AtomicLogLikelihood));
			});

		std::mutex Mutex;
		std::vector<double> LogLikelihoodSequence;
		for (std::size_t EM = 1; EM <= ExpectationMaximizationCount; EM++)
		{
			{
				std::unique_lock Lock(Mutex);
				ConditionVariable.wait(Lock);
			}

			TimeVector.push_back(std::chrono::system_clock::now());
			LogLikelihoodSequence.push_back(std::atomic_exchange(&AtomicLogLikelihood, 0));

			std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" <<
				" EM=" << EM << " Log-Likelihood=" << LogLikelihoodSequence.back() << " Improve=" << GetLastDifference(LogLikelihoodSequence) << std::endl;
		}
		std::for_each(std::begin(ThreadSequence), std::end(ThreadSequence), std::mem_fn(&std::thread::join));

		// ---------------- Score Probabilistic Latent Semantic Analysis ----------------

		auto QueriesDocumentsScore = std::vector<std::vector<double>>(QueriesCount, std::vector<double>(DocumentsCount, double(0)));
		for (std::size_t QueryIndex = 0; QueryIndex < QueriesCount; QueryIndex++)
		{
			for (std::size_t DocumentIndex = 0; DocumentIndex < DocumentsCount; DocumentIndex++)
			{
				QueriesDocumentsScore[QueryIndex][DocumentIndex] = GetScoreProbabilisticLatentSemanticAnalysis(
					QueriesWordCountMapSequence[QueryIndex],
					DocumentsWordCountMapSequence[DocumentIndex],
					DocumentsLengthSequence[DocumentIndex],
					DocumentsTopicProbabilitySequence[DocumentIndex],
					TopicsWordProbabilitySequence,
					BackgroundWordIndexMap,
					BackgroundWordCountMap,
					BackgroundLength,
					DocumentAlpha, DocumentBeta, DocumentGamma, DocumentBetaPrime, DocumentGammaPrime);
			}
		}

		TimeVector.push_back(std::chrono::system_clock::now());
		std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Core" << std::endl;

		// ---------------- Accumulate Index ----------------

		for (std::size_t QueryIndex = 0; QueryIndex < QueriesCount; QueryIndex++)
		{
			const auto SortedIndexSequence = SortIndex(QueriesDocumentsScore[QueryIndex], std::greater<double>());
			for (std::size_t DocumentIndex = 0; DocumentIndex < DocumentsCount; DocumentIndex++)
			{
				const auto& SortedIndex = SortedIndexSequence[DocumentIndex];
				QueriesDocumentsAccumulateIndex[QueryIndex][SortedIndex] += DocumentIndex;
			}
		}
	}

	// ---------------- Sort Score ----------------

	// Get File Stem
	auto QueriesStem = std::vector<std::string>(QueriesCount);
	std::transform(std::begin(QueriesPath), std::end(QueriesPath), std::begin(QueriesStem), GetPathStem);

	auto DocumentsStem = std::vector<std::string>(DocumentsCount);
	std::transform(std::begin(DocumentsPath), std::end(DocumentsPath), std::begin(DocumentsStem), GetPathStem);

	auto QueriesDocumentsSortedStem = std::vector<std::vector<std::reference_wrapper<const std::string>>>(QueriesCount);
	std::transform(std::begin(QueriesDocumentsAccumulateIndex), std::end(QueriesDocumentsAccumulateIndex), std::begin(QueriesDocumentsSortedStem),
		[&](const std::vector<std::size_t>& DocumentsAccumulateIndex)
		{
			return SortStem(DocumentsAccumulateIndex, DocumentsStem, OutputDocumentsCount, std::less<std::size_t>());
		});

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Sort Score" << std::endl;

	// ---------------- Output file ----------------

	// Output file
	std::stringstream OutputPath;
	OutputPath <<
		" Topic=" << TopicCount << " EM=" << ExpectationMaximizationCount <<
		" Alpha=" << DocumentAlpha << " Beta=" << DocumentBeta << " Gamma=" << DocumentGamma << " BetaPrime=" << DocumentBetaPrime << " GammaPrime=" << DocumentGammaPrime <<
		" PLSA=" << PLSASampleCount << " RandomSeed=" << UseRandomSeed <<
		" SpendTime=" << std::chrono::duration_cast<std::chrono::milliseconds>(TimeVector.back() - TimeVector.front()).count() << "ms" << OutputPathExtension;
	OutputResult(OutputPath.str(), OutputHeader, QueriesStem, QueriesDocumentsSortedStem);

	TimeVector.push_back(std::chrono::system_clock::now());
	std::cout << std::put_time(GetLocalTime(TimeVector.back()), "%c ") << std::chrono::nanoseconds(GetLastDifference(TimeVector)).count() << "ns" << " Output" << std::endl;

	// ---------------- End ----------------

	return 0;
}
